package fsm_test

import (
	"context"
	"errors"
	"os"
	"time"

	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"
	"github.com/rs/zerolog"

	. "github.com/Spirent/openperf/targets/spiperf/fsm"
	"github.com/Spirent/openperf/targets/spiperf/msg"
	"github.com/Spirent/openperf/targets/spiperf/openperf"
)

var _ = Describe("Server FSM,", func() {
	//Test-wide constants
	const assertEpsilon = time.Second * 15

	var (
		peerCmdIn    chan *msg.Message
		peerRespOut  chan *msg.Message
		peerNotifOut chan *msg.Message
		opCmdOut     chan *openperf.Command
		fsmReturn    chan error
		fsm          *Server
		ctx          context.Context
		cancelFunc   context.CancelFunc
		logger       zerolog.Logger
	)

	BeforeEach(func() {
		peerCmdIn = make(chan *msg.Message, 1)
		peerRespOut = make(chan *msg.Message, 1)
		peerNotifOut = make(chan *msg.Message, 1)
		opCmdOut = make(chan *openperf.Command)
		fsmReturn = make(chan error)
		// For more verbose output FatalLevel -> TraceLevel or InfoLevel
		logger = zerolog.New(zerolog.ConsoleWriter{Out: os.Stderr}).
			With().Logger().Level(zerolog.FatalLevel)

		fsm = &Server{
			Logger:                &logger,
			PeerCmdIn:             peerCmdIn,
			PeerRespOut:           peerRespOut,
			PeerNotifOut:          peerNotifOut,
			OpenperfCmdOut:        opCmdOut,
			PeerTimeout:           500 * time.Millisecond,
			OpenperfTimeout:       500 * time.Millisecond,
			StartDelay:            1 * time.Second,
			StatsPollInterval:     500 * time.Millisecond,
			GeneratorPollInterval: 500 * time.Millisecond,
		}

		ctx, cancelFunc = context.WithCancel(context.Background())

		go func() {
			fsmReturn <- fsm.Run(ctx)
		}()

	})

	Context("starts in the connect state, when peer sends a different version, ", func() {
		It("returns an error to peer, switches to connect state", func(done Done) {
			peerCmdIn <- &msg.Message{
				Type: msg.HelloType,
				Value: &msg.Hello{
					PeerProtocolVersion: "9.90",
				},
			}

			resp := <-peerRespOut

			Expect(resp).ToNot(BeNil())
			Expect(resp.Type).To(Equal(msg.ErrorType))
			Expect(resp.Value).ToNot(BeNil())
			Expect(fsm.State()).To(Equal("connect"))

			close(done)
		})

	})

	Context("starts in the connect state, when FSM is canceled via context, ", func() {
		It("terminates without error", func(done Done) {
			// Wait for FSM to start up.
			Eventually(func() string { return fsm.State() }).Should(Equal("connect"))

			cancelFunc()

			ret := <-fsmReturn
			Expect(ret).To(BeNil())
			Expect(fsm.State()).To(Equal("connect"))

			close(done)
		})
	})

	Context("starts in the connect state, when peer sends an unexpected message type, ", func() {
		It("returns an error to peer, switches to connect state", func(done Done) {
			peerCmdIn <- &msg.Message{
				Type:  msg.HelloType,
				Value: &msg.FinalStats{},
			}

			resp := <-peerRespOut

			Expect(resp).ToNot(BeNil())
			Expect(resp.Type).To(Equal(msg.ErrorType))
			Expect(resp.Value).ToNot(BeNil())
			Expect(fsm.State()).To(Equal("connect"))

			close(done)
		})
	})

	Context("starts in the connect state, when peer command channel returns an error, ", func() {
		It("terminates with an error", func(done Done) {
			// Wait for FSM to start up.
			Eventually(func() string { return fsm.State() }).Should(Equal("connect"))

			close(peerCmdIn)

			Eventually(fsmReturn).Should(Receive(BeAssignableToTypeOf(&PeerError{})))

			close(done)
		})

	})

	Context("starts in the connect state, peer receives hello message, peer sends hello reply, it transitions to the configure state, ", func() {
		BeforeEach(func(done Done) {
			peerCmdIn <- &msg.Message{
				Type: msg.HelloType,
				Value: &msg.Hello{
					PeerProtocolVersion: msg.Version,
				},
			}

			resp := <-peerRespOut
			Expect(resp).ToNot(BeNil())
			Expect(resp.Type).To(Equal(msg.HelloType))
			Expect(resp.Value).ToNot(BeNil())
			Expect(resp.Value).To(BeAssignableToTypeOf(&msg.Hello{}))
			Expect(resp.Value.(*msg.Hello).PeerProtocolVersion).To(Equal(msg.Version))

			Eventually(func() string { return fsm.State() }).Should(Equal("configure"))

			close(done)
		})

		Context("when client sends a command that's not GetConfigType, ", func() {
			It("returns to the connect state", func(done Done) {
				peerCmdIn <- &msg.Message{
					Type:  msg.HelloType,
					Value: &msg.FinalStats{},
				}

				resp := <-peerRespOut

				Expect(resp).ToNot(BeNil())
				Expect(resp.Type).To(Equal(msg.ErrorType))
				Expect(resp.Value).ToNot(BeNil())
				Eventually(func() string {
					return fsm.State()
				}, fsm.PeerTimeout.Seconds()*2).Should(Equal("connect"))

				close(done)
			})

		})

		Context("when client does not send get param message, ", func() {
			It("returns to the connect state", func(done Done) {

				resp := <-peerRespOut

				Expect(resp).ToNot(BeNil())
				Expect(resp.Type).To(Equal(msg.ErrorType))
				Expect(resp.Value).ToNot(BeNil())

				Eventually(func() string {
					return fsm.State()
				}, fsm.PeerTimeout.Seconds()*2).Should(Equal("connect"))

				close(done)
			})
		})

		Context("client requests server's parameters, server returns parameters, ", func() {
			BeforeEach(func(done Done) {
				peerCmdIn <- &msg.Message{
					Type: msg.GetServerParametersType,
				}

				resp := <-peerRespOut

				Expect(resp).ToNot(BeNil())
				Expect(resp.Type).To(Equal(msg.ServerParametersType))
				Expect(resp.Value).ToNot(BeNil())
				Expect(resp.Value).To(BeAssignableToTypeOf(&msg.ServerParametersResponse{}))

				close(done)
			}, assertEpsilon.Seconds())

			Context("when client does not send set config message, ", func() {
				It("returns to connect state", func(done Done) {

					resp := <-peerRespOut

					Expect(resp).ToNot(BeNil())
					Expect(resp.Type).To(Equal(msg.ErrorType))
					Expect(resp.Value).ToNot(BeNil())

					Eventually(func() string {
						return fsm.State()
					}, fsm.PeerTimeout.Seconds()*2).Should(Equal("connect"))

					close(done)

				}, assertEpsilon.Seconds())

			})

			Context("when client sends a command that's not SetConfigType, ", func() {
				It("returns to the connect state", func(done Done) {
					peerCmdIn <- &msg.Message{
						Type:  msg.HelloType,
						Value: &msg.FinalStats{},
					}

					resp := <-peerRespOut

					Expect(resp).ToNot(BeNil())
					Expect(resp.Type).To(Equal(msg.ErrorType))
					Expect(resp.Value).ToNot(BeNil())
					Eventually(func() string {
						return fsm.State()
					}, fsm.PeerTimeout.Seconds()*2).Should(Equal("connect"))

					close(done)
				})

			})

			// Note: this context exercises the server FSM code that sanity checks
			// values before sending them to Openperf.
			Context("when client sends invalid configuration, ", func() {
				It("returns to the connect state", func(done Done) {
					peerCmdIn <- &msg.Message{
						Type: msg.SetConfigType,
						Value: &msg.ServerConfiguration{
							DownstreamRateBps: 0,
							UpstreamRateBps:   0},
					}

					resp := <-peerRespOut

					Expect(resp).ToNot(BeNil())
					Expect(resp.Type).To(Equal(msg.ErrorType))
					Expect(resp.Value).ToNot(BeNil())
					Eventually(func() string {
						return fsm.State()
					}, fsm.PeerTimeout.Seconds()*2).Should(Equal("connect"))

					close(done)
				})
			})

			// At this point the test diverges along three paths based on test traffic flow:
			// * server -> client
			// * client -> server
			// * server <-> client
			// As an optimization the common error paths are only exercised on the
			//  server -> client path.
			// This needs to be done prior to returning the server parameters since
			//  the client builds the configuration immediately after getting this response.

			Context("client sends configuration, test has server -> client traffic, ", func() {
				BeforeEach(func(done Done) {

					peerCmdIn <- &msg.Message{
						Type: msg.SetConfigType,
						Value: &msg.ServerConfiguration{
							DownstreamRateBps: 100,
							UpstreamRateBps:   0},
					}

					close(done)
				})

				XContext("server configures Openperf, when an error occurs, ", func() {
					It("returns to the connect state", func(done Done) {
						//XXX: implement this once the server OP configuration commands get
						// implemented.

						close(done)
					})

				})

				Context("server configures Openperf, server ACKs configuration from client, it transitions to ready state, ", func() {
					BeforeEach(func(done Done) {

					Done:
						for {
							select {
							case opCmd := <-opCmdOut:
								close(opCmd.Done)

							case resp := <-peerRespOut:
								Expect(resp).ToNot(BeNil())
								Expect(resp.Type).To(Equal(msg.AckType))

								break Done
							}
						}

						Eventually(func() string {
							return fsm.State()
						}, fsm.PeerTimeout.Seconds()*2).Should(Equal("ready"))

						close(done)
					})

					Context("when client does not send the start command, ", func() {
						It("returns to the connect state", func(done Done) {
							resp := <-peerRespOut

							Expect(resp).ToNot(BeNil())
							Expect(resp.Type).To(Equal(msg.ErrorType))
							Expect(resp.Value).ToNot(BeNil())

							drainServerOpenperfCommands(opCmdOut, fsm)

							Eventually(func() string {
								return fsm.State()
							}, fsm.PeerTimeout.Seconds()*2).
								Should(Equal("connect"), "timed out waiting for FSM to return to connect state")

							close(done)
						}, assertEpsilon.Seconds())
					})

					Context("when there's an error instead of start command, ", func() {
						It("exits with an error", func(done Done) {

							close(peerCmdIn)

							drainServerOpenperfCommands(opCmdOut, fsm)

							Eventually(func() string {
								return fsm.State()
							}, fsm.PeerTimeout.Seconds()*2).
								Should(Equal("error"), "timed out waiting for FSM to switch to error state")

							Eventually(fsmReturn).Should(Receive(BeAssignableToTypeOf(&PeerError{})))

							close(done)
						}, assertEpsilon.Seconds())

					})

					Context("when the start command has an invalid time format, ", func() {
						It("returns to the connect state", func(done Done) {
							peerCmdIn <- &msg.Message{
								Type: msg.StartCommandType,
								Value: &msg.StartCommand{
									StartTime: "hello world"},
							}

							resp := <-peerRespOut

							Expect(resp).ToNot(BeNil())
							Expect(resp.Type).To(Equal(msg.ErrorType))
							Expect(resp.Value).ToNot(BeNil())

							drainServerOpenperfCommands(opCmdOut, fsm)

							Eventually(func() string {
								return fsm.State()
							}, fsm.PeerTimeout.Seconds()*2).
								Should(Equal("connect"), "timed out waiting for FSM to return to connect state")

							close(done)
						})

					})

					Context("when the start command has a time that's passed, ", func() {
						It("returns to the connect state, ", func(done Done) {

							peerCmdIn <- &msg.Message{
								Type: msg.StartCommandType,
								Value: &msg.StartCommand{
									StartTime: time.Now().Add(time.Hour * -1).Format(TimeFormatString)},
							}

							resp := <-peerRespOut

							Expect(resp).ToNot(BeNil())
							Expect(resp.Type).To(Equal(msg.ErrorType))
							Expect(resp.Value).ToNot(BeNil())

							drainServerOpenperfCommands(opCmdOut, fsm)

							Eventually(func() string {
								return fsm.State()
							}, fsm.PeerTimeout.Seconds()*2).
								Should(Equal("connect"), "timed out waiting for FSM to return to connect state")

							close(done)
						})

					})

					Context("when the start command has a time that's too far in the future, ", func() {
						It("returns to the connect state, ", func(done Done) {

							peerCmdIn <- &msg.Message{
								Type: msg.StartCommandType,
								Value: &msg.StartCommand{
									StartTime: time.Now().Add(time.Hour * 1).Format(TimeFormatString)},
							}

							resp := <-peerRespOut

							Expect(resp).ToNot(BeNil())
							Expect(resp.Type).To(Equal(msg.ErrorType))
							Expect(resp.Value).ToNot(BeNil())

							drainServerOpenperfCommands(opCmdOut, fsm)

							Eventually(func() string {
								return fsm.State()
							}, fsm.PeerTimeout.Seconds()*2).
								Should(Equal("connect"), "timed out waiting for FSM to return to connect state")

							close(done)
						})

					})

					Context("client sends start command, it transitions to armed state, ", func() {
						BeforeEach(func(done Done) {

							peerCmdIn <- &msg.Message{
								Type: msg.StartCommandType,
								Value: &msg.StartCommand{
									StartTime: time.Now().Add(time.Second * 2).Format(TimeFormatString)},
							}

							resp := <-peerRespOut
							Expect(resp).ToNot(BeNil())
							Expect(resp.Type).To(Equal(msg.AckType))

							Eventually(func() string {
								return fsm.State()
							}, fsm.PeerTimeout.Seconds()*2).
								Should(Equal("armed"), "timed out waiting for FSM to switch to armed state")

							close(done)
						})

						Context("when peer disconnects during armed state, ", func() {
							It("exits with an error", func(done Done) {

								peerCmdIn <- &msg.Message{
									Type:  msg.PeerDisconnectRemoteType,
									Value: &msg.PeerDisconnectRemoteNotif{Err: "aborting test."},
								}

								drainServerOpenperfCommands(opCmdOut, fsm)

								Eventually(func() string {
									return fsm.State()
								}, fsm.PeerTimeout.Seconds()*2).
									Should(Equal("connect"), "timed out waiting for FSM to return to connect state")

								close(done)
							}, assertEpsilon.Seconds())

						})

						Context(" when local peer interface errors during armed state, ", func() {
							It("exits with an error", func(done Done) {

								peerCmdIn <- &msg.Message{
									Type: msg.PeerDisconnectLocalType,
									Value: &msg.PeerDisconnectLocalNotif{
										Err: errors.New("unexpected local peer communcation error"),
									},
								}

								drainServerOpenperfCommands(opCmdOut, fsm)

								Eventually(func() string {
									return fsm.State()
								}, fsm.PeerTimeout.Seconds()*2).
									Should(Equal("connect"), "timed out waiting for FSM to return to connect state")

								close(done)
							}, assertEpsilon.Seconds())

						})

						// Remember, server -> client traffic here.
						Context("sleeps until start time, it transitions to running state, server is transmitting, ", func() {

							// At this point the test is running and we need to respond to
							// async requests from the server. Prior to this the Openperf requests
							// were done synchronously and the async responder wasn't necessary.
							// Also, the async responder will "drain" any requests that aren't poll
							// requests. Calling drainOpenperfCommands() is not needed and would
							// introduce timing issues.
							var (
								runstateStatsResponder       *openperfResponder
								txStatsResponderError        chan struct{}
								runstateResponderError       chan struct{}
								genDoneResponderError        chan struct{}
								runstateStatsResponderCancel context.CancelFunc
							)
							const runstatePollCount = 3

							BeforeEach(func(done Done) {

								txStatsResponderError = make(chan struct{})
								runstateResponderError = make(chan struct{})
								genDoneResponderError = make(chan struct{})
								runstateStatsResponder = &openperfResponder{
									OPCmdIn:           opCmdOut,
									ErrorRunstate:     runstateResponderError,
									ErrorTxStats:      txStatsResponderError,
									RunstatePollCount: runstatePollCount,
									ErrorDeleteGen:    genDoneResponderError,
								}
								var responderCtx context.Context
								responderCtx, runstateStatsResponderCancel = context.WithCancel(context.Background())
								go func() {
									defer GinkgoRecover()
									handleOpenperfResponses(responderCtx, runstateStatsResponder)
								}()

								// Server switches to running state here.

								Eventually(func() string {
									return fsm.State()
								}, time.Second*5).Should(Equal("running"))

								close(done)
							}, assertEpsilon.Seconds())

							Context("when client disconnects while test is running, ", func() {
								It("exits with an error", func(done Done) {
									peerCmdIn <- &msg.Message{
										Type: msg.PeerDisconnectRemoteType,
										Value: &msg.PeerDisconnectRemoteNotif{
											Err: "aborting test.",
										},
									}

									Eventually(func() string {
										return fsm.State()
									}, fsm.PeerTimeout.Seconds()*2).
										Should(Equal("connect"), "timed out waiting for FSM to return to connect state")

									close(done)

								}, assertEpsilon.Seconds())

							})

							Context("when openperf returns an error while polling generator, ", func() {
								It("exits with an error", func(done Done) {
									runstateResponderError <- struct{}{}

									Eventually(func() string {
										return fsm.State()
									}, fsm.PeerTimeout.Seconds()*2).
										Should(Equal("connect"), "timed out waiting for FSM to return to connect state")

									close(done)

								}, assertEpsilon.Seconds())

							})

							Context("when openperf returns an error while polling Tx stats, ", func() {
								It("exits with an error", func(done Done) {
									txStatsResponderError <- struct{}{}

									Eventually(func() string {
										return fsm.State()
									}, fsm.PeerTimeout.Seconds()*2).
										Should(Equal("connect"), "timed out waiting for FSM to return to connect state")

									close(done)

								}, assertEpsilon.Seconds())

							})

							Context("server sends notifications, test completes, it switches to the done state, ", func() {
								BeforeEach(func(done Done) {

								Done:
									for {
										notif := <-peerNotifOut
										Expect(notif).ToNot(BeNil())

										switch notif.Type {
										case msg.StatsNotificationType:
											//noop

										case msg.TransmitDoneType:
											break Done

										}

									}

									Eventually(func() string {
										return fsm.State()
									}).Should(Equal("done"), "timed out waiting for test to complete")

									close(done)
								}, assertEpsilon.Seconds())

								Context("when client does not send get final stats message, ", func() {
									It("returns to the connect state", func(done Done) {
										resp := <-peerRespOut

										Expect(resp).ToNot(BeNil())
										Expect(resp.Type).To(Equal(msg.ErrorType))
										Expect(resp.Value).ToNot(BeNil())

										Eventually(func() string {
											return fsm.State()
										}, fsm.PeerTimeout.Seconds()*2).
											Should(Equal("connect"), "timed out waiting for FSM to return to connect state")

										close(done)
									})

								})

								Context("client requests final stats, server sends final stats, it switches to the cleanup state, when Openperf errors with cleanup, ", func() {
									It("exits with an error", func(done Done) {

										genDoneResponderError <- struct{}{}

										peerCmdIn <- &msg.Message{
											Type: msg.GetFinalStatsType,
										}

										finalStats := <-peerRespOut

										Expect(finalStats).ToNot(BeNil())
										Expect(finalStats.Type).To(Equal(msg.FinalStatsType))
										Expect(finalStats.Value).To(BeAssignableToTypeOf(&msg.FinalStats{}))

										Eventually(fsmReturn).Should(Receive(
											BeAssignableToTypeOf(&OpenperfError{})))

										Expect(fsm.State()).To(Equal("error"))

										close(done)

									})
								})

								Context("server sends final stats, it switches to the cleanup state, when it cleans up, ", func() {
									It("returns without error.", func(done Done) {

										peerCmdIn <- &msg.Message{
											Type: msg.GetFinalStatsType,
										}

										finalStats := <-peerRespOut

										Expect(finalStats).ToNot(BeNil())
										Expect(finalStats.Type).To(Equal(msg.FinalStatsType))
										Expect(finalStats.Value).To(BeAssignableToTypeOf(&msg.FinalStats{}))

										Eventually(func() string {
											return fsm.State()
										}, fsm.PeerTimeout.Seconds()*2).
											Should(Equal("connect"), "timed out waiting for FSM to return to connect state")

										close(done)
									}, assertEpsilon.Seconds())
								})
							})

							AfterEach(func() {
								runstateStatsResponderCancel()
							})

						})
					})
				})
			})

			Context("server returns valid parameters, test has client -> server traffic, ", func() {
				BeforeEach(func(done Done) {

					peerCmdIn <- &msg.Message{
						Type: msg.SetConfigType,
						Value: &msg.ServerConfiguration{
							DownstreamRateBps: 0,
							UpstreamRateBps:   100},
					}

					close(done)
				})

				Context("server configures Openperf, server ACKs configuration from client, it transitions to ready state, ", func() {
					BeforeEach(func(done Done) {

					Done:
						for {
							select {
							case opCmd := <-opCmdOut:
								close(opCmd.Done)

							case resp := <-peerRespOut:
								Expect(resp).ToNot(BeNil())
								Expect(resp.Type).To(Equal(msg.AckType))

								break Done
							}
						}

						Eventually(func() string {
							return fsm.State()
						}, fsm.PeerTimeout.Seconds()*2).Should(Equal("ready"))

						close(done)
					})

					Context("client sends start command, it transitions to armed state, ", func() {
						BeforeEach(func(done Done) {

							peerCmdIn <- &msg.Message{
								Type: msg.StartCommandType,
								Value: &msg.StartCommand{
									StartTime: time.Now().Add(time.Second * 2).Format(TimeFormatString)},
							}

							resp := <-peerRespOut
							Expect(resp).ToNot(BeNil())
							Expect(resp.Type).To(Equal(msg.AckType))

							Eventually(func() string {
								return fsm.State()
							}, fsm.PeerTimeout.Seconds()*2).
								Should(Equal("armed"), "timed out waiting for FSM to switch to armed state")

							close(done)
						})

						// Remember, client -> server traffic here.
						Context("sleeps until start time, it transitions to running state, client is transmitting, ", func() {
							var runstateStatsResponder *openperfResponder
							var rxStatsResponderError chan struct{}
							const runstatePollCount = 3
							var runstateStatsResponderCancel context.CancelFunc
							//server starts up a runstate poll command here.
							BeforeEach(func(done Done) {

								rxStatsResponderError = make(chan struct{})
								runstateStatsResponder = &openperfResponder{
									OPCmdIn:           opCmdOut,
									ErrorRxStats:      rxStatsResponderError,
									RunstatePollCount: runstatePollCount,
								}
								var responderCtx context.Context
								responderCtx, runstateStatsResponderCancel = context.WithCancel(context.Background())
								go func() {
									defer GinkgoRecover()
									handleOpenperfResponses(responderCtx, runstateStatsResponder)
								}()
								// Server switches to running state here.

								Eventually(func() string { return fsm.State() }, fsm.PeerTimeout.Seconds()*8).Should(Equal("running"), "timed out waiting for FSM to switch to running state")

								close(done)
							}, assertEpsilon.Seconds())

							Context("when openperf returns an error while polling Rx stats, ", func() {
								It("exits with an error", func(done Done) {
									rxStatsResponderError <- struct{}{}

									Eventually(func() string {
										return fsm.State()
									}, fsm.PeerTimeout.Seconds()*2).
										Should(Equal("connect"), "timed out waiting for FSM to return to connect state")

									close(done)

								}, assertEpsilon.Seconds())

							})

							Context("server sends notifications, test completes, it switches to the done state, ", func() {
								BeforeEach(func(done Done) {

									peerNotifCount, expectedNotifCount := 0, 3

								Done:
									for {
										notif := <-peerNotifOut
										Expect(notif).ToNot(BeNil())
										Expect(notif.Type).To(Equal(msg.StatsNotificationType))
										Expect(notif.Value).ToNot(BeNil())
										Expect(notif.Value).To(BeAssignableToTypeOf(&msg.DataStreamStats{}))

										peerNotifCount++

										if peerNotifCount == expectedNotifCount {
											peerCmdIn <- &msg.Message{
												Type: msg.TransmitDoneType}

											break Done
										}

									}

									Eventually(func() string {
										return fsm.State()
									}).Should(Equal("done"), "timed out waiting for FSM to transition to done state")

									close(done)
								}, assertEpsilon.Seconds())

								Context("server sends final stats, ", func() {
									It("switches to the cleanup state", func(done Done) {
										peerCmdIn <- &msg.Message{
											Type: msg.GetFinalStatsType,
										}

										finalStats := <-peerRespOut

										Expect(finalStats).ToNot(BeNil())
										Expect(finalStats.Type).To(Equal(msg.FinalStatsType))
										Expect(finalStats.Value).To(BeAssignableToTypeOf(&msg.FinalStats{}))

										Eventually(func() string {
											return fsm.State()
										}, fsm.PeerTimeout.Seconds()*2).
											Should(Equal("connect"), "timed out waiting for FSM to return to connect state")

										close(done)
									}, assertEpsilon.Seconds())

								})
							})

							AfterEach(func() {
								runstateStatsResponderCancel()
							})
						})
					})
				})
			})

			Context("server returns valid parameters, test has server <-> client traffic, ", func() {
				BeforeEach(func(done Done) {

					peerCmdIn <- &msg.Message{
						Type: msg.SetConfigType,
						Value: &msg.ServerConfiguration{
							DownstreamRateBps: 100,
							UpstreamRateBps:   100},
					}

					close(done)
				})
				Context("server configures Openperf, server ACKs configuration from client, it transitions to ready state, ", func() {

					BeforeEach(func(done Done) {
					Done:
						for {
							select {
							case opCmd := <-opCmdOut:
								close(opCmd.Done)

							case resp := <-peerRespOut:
								Expect(resp).ToNot(BeNil())
								Expect(resp.Type).To(Equal(msg.AckType))

								break Done
							}
						}

						Eventually(func() string {
							return fsm.State()
						}, fsm.PeerTimeout.Seconds()*2).Should(Equal("ready"))

						close(done)
					})

					Context("client sends start command, it transitions to armed state, ", func() {
						BeforeEach(func(done Done) {

							peerCmdIn <- &msg.Message{
								Type: msg.StartCommandType,
								Value: &msg.StartCommand{
									StartTime: time.Now().Add(time.Second * 2).Format(TimeFormatString)},
							}

							resp := <-peerRespOut
							Expect(resp).ToNot(BeNil())
							Expect(resp.Type).To(Equal(msg.AckType))

							Eventually(func() string {
								return fsm.State()
							}, fsm.PeerTimeout.Seconds()*2).
								Should(Equal("armed"), "timed out waiting for FSM to switch to armed state")

							close(done)
						})

						// Remember, client <-> server traffic here.
						Context("sleeps until start time, it transitions to running state, server and client are transmitting, ", func() {
							var runstateStatsResponder *openperfResponder
							var setRunstatePollCount chan int
							const runstatePollCount = 30 //This will be reduced by the channel above.
							var runstateStatsResponderCancel context.CancelFunc
							//client starts up a runstate poll command here.
							BeforeEach(func(done Done) {

								setRunstatePollCount = make(chan int)
								runstateStatsResponder = &openperfResponder{
									OPCmdIn:              opCmdOut,
									RunstatePollCount:    runstatePollCount,
									NewRunstatePollCount: setRunstatePollCount,
								}
								var responderCtx context.Context
								responderCtx, runstateStatsResponderCancel = context.WithCancel(context.Background())
								go func() {
									defer GinkgoRecover()
									handleOpenperfResponses(responderCtx, runstateStatsResponder)
								}()
								// Client switches to running state here.
								Eventually(func() string {
									return fsm.State()
								}, fsm.PeerTimeout.Seconds()*8).
									Should(Equal("running"), "timed out waiting for FSM to switch to running state")

								close(done)
							}, assertEpsilon.Seconds())

							// At this point either server or client will stop first.
							// Both are tested below for completeness

							// client stops first
							Context("client sends notifications, client stops transmitting, ", func() {
								BeforeEach(func(done Done) {

									peerNotifCount, expectedNotifCount := 0, 3

								Done:
									for {
										notif := <-peerNotifOut
										Expect(notif).ToNot(BeNil())
										Expect(notif.Type).To(Equal(msg.StatsNotificationType))
										Expect(notif.Value).ToNot(BeNil())
										Expect(notif.Value).To(BeAssignableToTypeOf(&msg.DataStreamStats{}))

										peerNotifCount++

										if peerNotifCount == expectedNotifCount {
											peerCmdIn <- &msg.Message{
												Type: msg.TransmitDoneType}

											break Done
										}

									}

									Expect(fsm.State()).To(Equal("running"))

									close(done)
								}, assertEpsilon.Seconds())

								Context("server continues to run, server stops, ", func() {
									BeforeEach(func(done Done) {

										// Let test run a bit more.
										timerChan := time.After(1 * time.Second)
									Done:
										for {
											select {
											case notif := <-peerNotifOut:
												Expect(notif).ToNot(BeNil())

												switch notif.Type {
												case msg.StatsNotificationType:

												case msg.TransmitDoneType:
													break Done

												}
											case <-timerChan:
												setRunstatePollCount <- int(0)
											}
										}

										Eventually(func() string {
											return fsm.State()
										}).Should(Equal("done"))

										close(done)
									}, assertEpsilon.Seconds())

									Context("server sends final stats, ", func() {
										It("switches to the cleanup state", func(done Done) {

											peerCmdIn <- &msg.Message{
												Type: msg.GetFinalStatsType,
											}

											finalStats := <-peerRespOut

											Expect(finalStats).ToNot(BeNil())
											Expect(finalStats.Type).To(Equal(msg.FinalStatsType))
											Expect(finalStats.Value).To(BeAssignableToTypeOf(&msg.FinalStats{}))

											Eventually(func() string {
												return fsm.State()
											}, fsm.PeerTimeout.Seconds()*2).
												Should(Equal("connect"), "timed out waiting for FSM to return to connect state")

											runstateStatsResponderCancel()

											close(done)

										}, assertEpsilon.Seconds())

									})

								})
							})

							// server stops first.
							Context("server sends notifications, server stops transmitting, ", func() {
								BeforeEach(func(done Done) {
									peerNotifCount, expectedNotifCount := 0, 3

								Done:
									for {
										notif := <-peerNotifOut
										Expect(notif).ToNot(BeNil())
										Expect(notif.Type).To(Equal(msg.StatsNotificationType))
										Expect(notif.Value).ToNot(BeNil())
										Expect(notif.Value).To(BeAssignableToTypeOf(&msg.DataStreamStats{}))

										peerNotifCount++

										if peerNotifCount == expectedNotifCount {
											setRunstatePollCount <- int(0)

											break Done
										}

									}

									setRunstatePollCount <- int(0)

								DoneAgain:
									for {
										notif := <-peerNotifOut
										Expect(notif).NotTo(BeNil())

										switch notif.Type {
										case msg.StatsNotificationType:
											//noop

										case msg.TransmitDoneType:
											break DoneAgain

										}
									}

									Expect(fsm.State()).To(Equal("running"))

									close(done)
								}, assertEpsilon.Seconds())

								Context("client continues to run, client stops, ", func() {
									BeforeEach(func(done Done) {

										// Run for a bit more.
										timerChan := time.After(1 * time.Second)
									Done:
										for {
											select {
											case notif := <-peerNotifOut:
												Expect(notif).ToNot(BeNil())
												Expect(notif.Type).To(Equal(msg.StatsNotificationType))

											case <-timerChan:
												peerCmdIn <- &msg.Message{
													Type: msg.TransmitDoneType}

												break Done
											}
										}

										close(done)
									}, assertEpsilon.Seconds())

									Context("client requests final stats, server sends final stats, ", func() {
										It("switches to the cleanup state", func(done Done) {

											peerCmdIn <- &msg.Message{Type: msg.GetFinalStatsType}

										Done:
											for {
												select {
												case notif := <-peerNotifOut:
													Expect(notif).ToNot(BeNil())
													Expect(notif.Type).To(Equal(msg.StatsNotificationType))

												case resp := <-peerRespOut:
													Expect(resp).ToNot(BeNil())
													Expect(resp.Type).To(Equal(msg.FinalStatsType))
													Expect(resp.Value).To(BeAssignableToTypeOf(&msg.FinalStats{}))

													break Done
												}
											}

											Eventually(func() string {
												return fsm.State()
											}).Should(Equal("connect"))

											close(done)

										}, assertEpsilon.Seconds())

									})
								})
							})

							AfterEach(func(done Done) {
								runstateStatsResponderCancel()
								close(done)
							})
						})
					})
				})
			})
		})
	})

	AfterEach(func() {
		cancelFunc()
	})
})

// Unit test utilities

func drainServerOpenperfCommands(opCmd chan *openperf.Command, fsm *Server) {

	//Rather than having a static time window for OP commands, or waiting for a specific cleanup
	// message, poll the FSM state to see if we've gotten through the cleanup state.
	//Server behaves slightly differently in that it will not terminate in the cleanup state.
	fsmDone := make(chan struct{})
	go func() {
		for {
			state := fsm.State()

			switch state {
			case "connect":
				close(fsmDone)
				return
			case "error":
				close(fsmDone)
				return
			default:

			}

			time.Sleep(10 * time.Millisecond)
		}

	}()

	for {
		select {
		case <-fsmDone:
			return
		case cmd := <-opCmd:
			close(cmd.Done)
		}
	}
}
