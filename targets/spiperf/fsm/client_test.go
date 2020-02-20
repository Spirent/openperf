package fsm_test

import (
	"context"
	"errors"
	"fmt"
	"time"

	"github.com/go-openapi/strfmt"
	"github.com/go-openapi/strfmt/conv"
	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"

	. "github.com/Spirent/openperf/targets/spiperf/fsm"
	"github.com/Spirent/openperf/targets/spiperf/msg"
	"github.com/Spirent/openperf/targets/spiperf/openperf"
	"github.com/Spirent/openperf/targets/spiperf/openperf/v1/models"
)

var _ = Describe("Client FSM,", func() {
	//Test-wide constants
	const assertEpsilon = time.Second * 15

	var (
		peerCmdOut  chan *msg.Message
		peerRespIn  chan *msg.Message
		peerNotifIn chan *msg.Message
		opCmdOut    chan *openperf.Command
		fsmReturn   chan error
		fsm         *Client
		ctx         context.Context
		cancelFunc  context.CancelFunc
	)

	BeforeEach(func() {
		peerCmdOut = make(chan *msg.Message, 1)
		peerRespIn = make(chan *msg.Message, 1)
		peerNotifIn = make(chan *msg.Message, 1)
		opCmdOut = make(chan *openperf.Command)
		fsmReturn = make(chan error)

		fsm = &Client{
			PeerCmdOut:            peerCmdOut,
			PeerRespIn:            peerRespIn,
			PeerNotifIn:           peerNotifIn,
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

	Context("starts in the connect state, ", func() {
		BeforeEach(func(done Done) {

			command := <-peerCmdOut

			Expect(command).NotTo(BeNil())
			Expect(command.Type).To(Equal(msg.HelloType))
			Expect(command.Value).To(BeAssignableToTypeOf(&msg.Hello{}))
			hello := &msg.Hello{
				Version: Version,
			}
			Expect(command.Value).To(Equal(hello))

			Expect(fsm.State()).To(Equal("connect"))
			close(done)
		}, assertEpsilon.Seconds())

		Context("when peer responds with a different version, ", func() {
			It("terminates with a version mismatch error", func(done Done) {
				peerRespIn <- &msg.Message{
					Type: msg.HelloType,
					Value: &msg.Hello{
						Version: "9.90",
					},
				}

				Eventually(fsmReturn).Should(Receive(BeAssignableToTypeOf(&InternalError{})))
				Expect(peerCmdOut).To(BeClosed())

				close(done)
			})

		})

		Context("when peer returns an error, ", func() {
			It("terminates with an error", func(done Done) {
				peerRespIn <- &msg.Message{
					Type:  msg.ErrorType,
					Value: "server error",
				}

				Eventually(fsmReturn).Should(Receive(BeAssignableToTypeOf(&InternalError{})))
				Expect(peerCmdOut).To(BeClosed())

				close(done)
			})
		})

		Context("when peer does not respond to hello message", func() {
			It("terminates with an error", func(done Done) {
				Eventually(fsmReturn).Should(Receive(BeAssignableToTypeOf(&TimeoutError{})))
				Expect(peerCmdOut).To(BeClosed())
				close(done)
			})
		})

		Context("when peer response channel returns an error", func() {
			It("terminates with an error", func(done Done) {
				close(peerRespIn)
				Eventually(fsmReturn).Should(Receive(BeAssignableToTypeOf(&InternalError{})))
				Expect(peerCmdOut).To(BeClosed())
				close(done)
			})

		})

		Context("peer returns server parameters, it transitions to the configure state, ", func() {
			BeforeEach(func(done Done) {
				peerRespIn <- &msg.Message{
					Type: msg.HelloType,
					Value: &msg.Hello{
						Version: Version,
					},
				}

				command := <-peerCmdOut
				Expect(command).NotTo(BeNil())
				Expect(command.Type).To(Equal(msg.GetConfigType))
				Expect(command.Value).To(BeNil())

				Expect(fsm.State()).To(Equal("configure"))

				close(done)
			})

			Context("when server returns an error, ", func() {
				It("terminates with an error", func(done Done) {
					peerRespIn <- &msg.Message{
						Type:  msg.ErrorType,
						Value: "server error",
					}

					Eventually(fsmReturn).Should(Receive(BeAssignableToTypeOf(&InternalError{})))
					Expect(peerCmdOut).To(BeClosed())
					close(done)
				})
			})

			Context("when server returns invalid parameters, ", func() {
				It("terminates with an error", func(done Done) {
					peerRespIn <- &msg.Message{
						Type: msg.ServerParametersResponseType,
						Value: &msg.ServerParametersResponse{
							OpenperfURL: ":http://localhost:9000",
						},
					}

					Eventually(fsmReturn).Should(Receive(
						BeAssignableToTypeOf(&InvalidConfigurationError{})))
					Expect(peerCmdOut).To(BeClosed())

					close(done)
				})
			})

			Context("when server does not respond to get param message", func() {
				It("terminates with an error", func(done Done) {
					Eventually(fsmReturn).Should(Receive(BeAssignableToTypeOf(&TimeoutError{})))
					Expect(peerCmdOut).To(BeClosed())
					close(done)
				})
			})

			// At this point the test diverges along three paths based on test traffic flow:
			// * server -> client
			// * client -> server
			// * server <-> client
			// As an optimization the common error paths are only exercised on the
			//  client -> server path.
			// This needs to be done prior to returning the server parameters since
			//  the client builds the configuration immediately after getting this response.

			Context("server returns valid parameters, test has client -> server traffic, ", func() {
				BeforeEach(func() {

					fsm.TestConfiguration.UpstreamRate = 100
					fsm.TestConfiguration.DownstreamRate = 0

					peerRespIn <- &msg.Message{
						Type: msg.ServerParametersResponseType,
						Value: &msg.ServerParametersResponse{
							OpenperfURL: "http://localhost:9000",
						},
					}

					command := <-peerCmdOut
					Expect(command).ToNot(BeNil())
					Expect(command.Type).To(Equal(msg.SetConfigType))
					Expect(command.Value).ToNot(BeNil())
					serverCfg := &msg.ServerConfiguration{}
					Expect(command.Value).To(Equal(serverCfg))
				})

				Context("when server NACKs configuration from client, ", func() {
					It("exits with an error", func(done Done) {
						peerRespIn <- &msg.Message{
							Type:  msg.ErrorType,
							Value: "error with configuration",
						}

						drainCleanupRequests(opCmdOut)

						Eventually(fsmReturn).Should(Receive(
							BeAssignableToTypeOf(&InvalidConfigurationError{})))
						Expect(peerCmdOut).To(BeClosed())
						Expect(fsm.State()).To(Equal("cleanup"))

						close(done)
					}, assertEpsilon.Seconds())
				})

				Context("server ACKs configuration from client, it transitions to ready state, ", func() {
					var openperfGetTimeCmd *openperf.Command
					BeforeEach(func() {
						//Ack setconfig
						peerRespIn <- &msg.Message{
							Type: msg.AckType,
						}

						Eventually(func() string { return fsm.State() }).Should(Equal("ready"))

						//Openperf get time command
						openperfGetTimeCmd = <-opCmdOut
						Expect(openperfGetTimeCmd).ToNot(BeNil())
						Expect(openperfGetTimeCmd.Request).To(BeAssignableToTypeOf(&openperf.GetTimeRequest{}))
						Expect(openperfGetTimeCmd.Done).ToNot(BeNil())
						Expect(openperfGetTimeCmd.Response).To(BeNil())
					})

					Context("when openperf command times out, ", func() {
						It("exits with an error", func(done Done) {
							// Openperf requests will never instantaneously time out.
							// Sleep here for a little while to more accurately reflect reality.
							time.Sleep(10 * time.Millisecond)
							openperfGetTimeCmd.Response = context.DeadlineExceeded
							close(openperfGetTimeCmd.Done)

							drainCleanupRequests(opCmdOut)

							Eventually(fsmReturn).Should(Receive(
								BeAssignableToTypeOf(&InternalError{})))
							Expect(peerCmdOut).To(BeClosed())
							Expect(fsm.State()).To(Equal("cleanup"))

							close(done)

						})

					})

					Context("when openperf controller returns an error, ", func() {
						It("exits with an error", func(done Done) {
							openperfGetTimeCmd.Response = errors.New("error getting command from Openperf")
							close(openperfGetTimeCmd.Done)

							drainCleanupRequests(opCmdOut)

							Eventually(fsmReturn).Should(
								Receive(BeAssignableToTypeOf(&InternalError{})))
							Expect(peerCmdOut).To(BeClosed())
							Expect(fsm.State()).To(Equal("cleanup"))

							close(done)
						})
					})

					Context("openperf returns the current time, ", func() {
						var startCmd *msg.Message
						BeforeEach(func(done Done) {
							openperfGetTimeCmd.Response = &models.TimeKeeper{Time: conv.DateTime(strfmt.DateTime(time.Now()))}
							close(openperfGetTimeCmd.Done)

							startCmd = <-peerCmdOut

							Expect(startCmd).ToNot(BeNil())
							Expect(startCmd.Type).To(Equal(msg.StartCommandType))
							Expect(startCmd.Value).ToNot(BeNil())
							Expect(startCmd.Value).To(BeAssignableToTypeOf(&msg.StartCommand{}))
							Expect(startCmd.Value.(*msg.StartCommand).StartTime).To(BeTemporally("~", time.Now().Add(fsm.StartDelay), time.Millisecond*250))

							close(done)
						})

						Context("when server NACKs start command, ", func() {
							It("exits with an error", func(done Done) {
								peerRespIn <- &msg.Message{
									Type:  msg.ErrorType,
									Value: "error with configuration",
								}

								drainCleanupRequests(opCmdOut)

								Eventually(fsmReturn).Should(Receive(
									BeAssignableToTypeOf(&InvalidConfigurationError{})))
								Expect(peerCmdOut).To(BeClosed())
								Expect(fsm.State()).To(Equal("cleanup"))

								close(done)

							})
						})

						Context("server ACKs start command, it transitions to armed state, sleeps until start time, it transitions to runningTx state, client is transmitting, ", func() {
							var runstateStatsResponder *openperfResponder
							var txStatsResponderError chan struct{}
							var runstateResponderError chan struct{}
							const runstatePollCount = 3
							var runstateStatsResponderCancel context.CancelFunc
							//client starts up a runstate poll command here.
							BeforeEach(func(done Done) {
								//ACK start command.
								peerRespIn <- &msg.Message{
									Type: msg.AckType,
								}

								Eventually(func() string {
									return fsm.State()
								}).Should(Equal("armed"))

								// Client sleeps here until start time.
								time.Sleep(time.Until(startCmd.Value.(*msg.StartCommand).StartTime))

								txStatsResponderError = make(chan struct{})
								runstateResponderError = make(chan struct{})
								runstateStatsResponder = &openperfResponder{
									OPCmdIn:           opCmdOut,
									ErrorRunstate:     runstateResponderError,
									ErrorTxStats:      txStatsResponderError,
									RunstatePollCount: runstatePollCount,
								}
								var responderCtx context.Context
								responderCtx, runstateStatsResponderCancel = context.WithCancel(context.Background())
								go handleOpenperfResponses(responderCtx, runstateStatsResponder)

								// Client switches to running state here.

								Eventually(func() string {
									return fsm.State()
								}).Should(Equal("running"))

								close(done)
							}, assertEpsilon.Seconds())

							Context("when server sends an error on notification channel, ", func() {
								It("exits with an error", func(done Done) {
									peerNotifIn <- &msg.Message{
										Type:  msg.ErrorType,
										Value: "error with rx stats",
									}

									drainCleanupRequests(opCmdOut)

									Eventually(fsmReturn).Should(Receive(
										BeAssignableToTypeOf(&InternalError{})))
									Expect(peerCmdOut).To(BeClosed())
									Expect(fsm.State()).To(Equal("cleanup"))

									close(done)
								})
							})

							Context("when openperf returns an error while polling generator, ", func() {
								It("exits with an error", func(done Done) {
									runstateResponderError <- struct{}{}

									drainCleanupRequests(opCmdOut)

									val := <-fsmReturn
									Expect(val).To(BeAssignableToTypeOf(&InternalError{}))
									Expect(peerCmdOut).To(BeClosed())
									Expect(fsm.State()).To(Equal("cleanup"))

									close(done)

								}, assertEpsilon.Seconds())

							})

							Context("when openperf returns an error while polling Tx stats, ", func() {
								It("exits with an error", func(done Done) {
									txStatsResponderError <- struct{}{}

									drainCleanupRequests(opCmdOut)

									val := <-fsmReturn
									Expect(val).To(BeAssignableToTypeOf(&InternalError{}))
									Expect(peerCmdOut).To(BeClosed())
									Expect(fsm.State()).To(Equal("cleanup"))

									close(done)

								}, assertEpsilon.Seconds())

							})

							Context("server sends notifications, test completes, it switches to the done state, ", func() {
								BeforeEach(func(done Done) {
									statsNotifDone := make(chan struct{})
									go func() {
										for {
											select {
											case <-statsNotifDone:
												return
											case <-time.After(1 * time.Second):
												peerNotifIn <- &msg.Message{
													Type: msg.StatsNotificationType,
													Value: &msg.RuntimeStats{
														Timestamp: uint64(time.Now().Unix()),
													},
												}
											}
										}
									}()

									cmd := <-peerCmdOut
									Expect(cmd).NotTo(BeNil())
									Expect(cmd.Type).To(Equal(msg.TransmitDoneType))
									Expect(cmd.Value).To(BeNil())
									close(statsNotifDone)

									runstateStatsResponderCancel()

									Eventually(func() string {
										return fsm.State()
									}).Should(Equal("done"))

									cmd = <-peerCmdOut
									Expect(cmd).NotTo(BeNil())
									Expect(cmd.Type).To(Equal(msg.GetFinalStatsType))
									Expect(cmd.Value).To(BeNil())

									close(done)
								}, assertEpsilon.Seconds())

								Context("when server sends an error instead of final stats, ", func() {
									It("exits with an error", func(done Done) {
										peerRespIn <- &msg.Message{
											Type:  msg.ErrorType,
											Value: "error gathering final stats"}

										drainCleanupRequests(opCmdOut)

										Eventually(fsmReturn).Should(Receive(
											BeAssignableToTypeOf(&MessagingError{})))
										Expect(peerCmdOut).To(BeClosed())
										Expect(fsm.State()).To(Equal("cleanup"))

										close(done)
									})

								})

								Context("server sends final stats, it switches to the cleanup state, ", func() {
									BeforeEach(func(done Done) {
										peerRespIn <- &msg.Message{
											Type: msg.FinalStatsType,
											Value: &msg.FinalStats{
												TransmitFrames: uint64(420)},
										}

										Eventually(func() string {
											return fsm.State()
										}).Should(Equal("cleanup"))

										close(done)
									})

									Context("when Openperf errors with cleanup, ", func() {
										It("exits with an error", func(done Done) {
											req := <-opCmdOut
											req.Response = errors.New("error cleaning up")
											close(req.Done)

											Eventually(fsmReturn).Should(Receive(
												BeAssignableToTypeOf(&InternalError{})))
											Expect(peerCmdOut).To(BeClosed())
											Expect(fsm.State()).To(Equal("cleanup"))

											close(done)
										})

									})

									Context("it cleans up, ", func() {
										It("returns without error.", func(done Done) {
											req := <-opCmdOut
											Expect(req).NotTo(BeNil())
											Expect(req.Request).To(BeAssignableToTypeOf(&openperf.DeleteGeneratorRequest{}))
											Expect(req.Ctx).ToNot(BeNil())
											Expect(req.Done).ToNot(BeNil())
											Expect(req.Response).To(BeNil())

											close(req.Done)

											res := <-fsmReturn
											Expect(res).To(BeNil())

											Expect(fsm.State()).To(Equal("cleanup"))

											close(done)
										}, assertEpsilon.Seconds())
									})
								})
							})

							AfterEach(func() {
								runstateStatsResponderCancel()
							})

						})
					})
				})
			})

			Context("server returns valid parameters, test has server -> client traffic, ", func() {
				BeforeEach(func() {

					fsm.TestConfiguration.UpstreamRate = 0
					fsm.TestConfiguration.DownstreamRate = 100

					peerRespIn <- &msg.Message{
						Type: msg.ServerParametersResponseType,
						Value: &msg.ServerParametersResponse{
							OpenperfURL: "http://localhost:9000",
						},
					}

					command := <-peerCmdOut
					Expect(command).ToNot(BeNil())
					Expect(command.Type).To(Equal(msg.SetConfigType))
					Expect(command.Value).ToNot(BeNil())
					serverCfg := &msg.ServerConfiguration{}
					Expect(command.Value).To(Equal(serverCfg))
				})

				Context("server ACKs configuration from client, it transitions to ready state, ", func() {
					var openperfGetTimeCmd *openperf.Command
					BeforeEach(func() {
						//Ack setconfig
						peerRespIn <- &msg.Message{
							Type: msg.AckType,
						}

						Eventually(func() string { return fsm.State() }).Should(Equal("ready"))

						//Openperf get time command
						openperfGetTimeCmd = <-opCmdOut
						Expect(openperfGetTimeCmd).ToNot(BeNil())
						Expect(openperfGetTimeCmd.Request).To(BeAssignableToTypeOf(&openperf.GetTimeRequest{}))
						Expect(openperfGetTimeCmd.Done).ToNot(BeNil())
						Expect(openperfGetTimeCmd.Response).To(BeNil())
					})

					Context("openperf returns the current time, ", func() {
						var startCmd *msg.Message
						BeforeEach(func(done Done) {
							openperfGetTimeCmd.Response = &models.TimeKeeper{Time: conv.DateTime(strfmt.DateTime(time.Now()))}
							close(openperfGetTimeCmd.Done)

							startCmd = <-peerCmdOut

							Expect(startCmd).ToNot(BeNil())
							Expect(startCmd).ToNot(BeNil())
							Expect(startCmd.Type).To(Equal(msg.StartCommandType))
							Expect(startCmd.Value).ToNot(BeNil())
							Expect(startCmd.Value).To(BeAssignableToTypeOf(&msg.StartCommand{}))

							close(done)
						})

						Context("server ACKs start command, it transitions to armed state, sleeps until start time, it transitions to running state, server is transmitting, ", func() {
							var runstateStatsResponder *openperfResponder
							var rxStatsResponderError chan struct{}
							const runstatePollCount = 3
							var runstateStatsResponderCancel context.CancelFunc
							//client starts up a runstate poll command here.
							BeforeEach(func(done Done) {
								//ACK start command.
								peerRespIn <- &msg.Message{
									Type: msg.AckType,
								}

								// Client sleeps here until start time.
								time.Sleep(time.Until(startCmd.Value.(*msg.StartCommand).StartTime))

								rxStatsResponderError = make(chan struct{})
								runstateStatsResponder = &openperfResponder{
									OPCmdIn:           opCmdOut,
									ErrorRxStats:      rxStatsResponderError,
									RunstatePollCount: runstatePollCount,
								}
								var responderCtx context.Context
								responderCtx, runstateStatsResponderCancel = context.WithCancel(context.Background())
								go handleOpenperfResponses(responderCtx, runstateStatsResponder)

								// Client switches to running state here.

								close(done)
							}, assertEpsilon.Seconds())

							Context("when openperf returns an error while polling Rx stats, ", func() {
								It("exits with an error", func(done Done) {
									rxStatsResponderError <- struct{}{}

									val := <-fsmReturn
									Expect(val).To(BeAssignableToTypeOf(&InternalError{}))
									Expect(peerCmdOut).To(BeClosed())
									Expect(fsm.State()).To(Equal("cleanup"))

									close(done)

								}, assertEpsilon.Seconds())

							})

							Context("server sends notifications, test completes, it switches to the done state, ", func() {
								BeforeEach(func(done Done) {

									for i := 0; i < 4; i++ {
										peerNotifIn <- &msg.Message{
											Type: msg.StatsNotificationType,
											Value: &msg.RuntimeStats{
												Timestamp: uint64(time.Now().Unix()),
											},
										}
										time.Sleep(500 * time.Millisecond)
									}

									peerNotifIn <- &msg.Message{
										Type: msg.TransmitDoneType,
									}

									runstateStatsResponderCancel()

									Eventually(func() string {
										return fsm.State()
									}).Should(Equal("done"))

									cmd := <-peerCmdOut
									Expect(cmd).NotTo(BeNil())
									Expect(cmd.Type).To(Equal(msg.GetFinalStatsType))
									Expect(cmd.Value).To(BeNil())

									close(done)
								}, assertEpsilon.Seconds())

								Context("server sends final stats, ", func() {
									It("switches to the cleanup state", func(done Done) {
										peerRespIn <- &msg.Message{
											Type: msg.FinalStatsType,
											Value: &msg.FinalStats{
												TransmitFrames: uint64(420)},
										}

										Eventually(func() string {
											return fsm.State()
										}).Should(Equal("cleanup"))

										close(done)
									})

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
				BeforeEach(func() {

					fsm.TestConfiguration.UpstreamRate = 100
					fsm.TestConfiguration.DownstreamRate = 100

					peerRespIn <- &msg.Message{
						Type: msg.ServerParametersResponseType,
						Value: &msg.ServerParametersResponse{
							OpenperfURL: "http://localhost:9000",
						},
					}

					command := <-peerCmdOut
					Expect(command).ToNot(BeNil())
					Expect(command.Type).To(Equal(msg.SetConfigType))
					Expect(command.Value).ToNot(BeNil())
					serverCfg := &msg.ServerConfiguration{}
					Expect(command.Value).To(Equal(serverCfg))
				})
				Context("server ACKs configuration from client, it transitions to ready state, ", func() {
					var openperfGetTimeCmd *openperf.Command
					BeforeEach(func() {
						//Ack setconfig
						peerRespIn <- &msg.Message{
							Type: msg.AckType,
						}

						Eventually(func() string { return fsm.State() }).Should(Equal("ready"))

						//Openperf get time command
						openperfGetTimeCmd = <-opCmdOut
					})

					Context("openperf returns the current time, ", func() {
						var startCmd *msg.Message
						BeforeEach(func(done Done) {
							openperfGetTimeCmd.Response = &models.TimeKeeper{Time: conv.DateTime(strfmt.DateTime(time.Now()))}
							close(openperfGetTimeCmd.Done)

							startCmd = <-peerCmdOut

							Expect(startCmd).ToNot(BeNil())
							Expect(startCmd).ToNot(BeNil())
							Expect(startCmd.Type).To(Equal(msg.StartCommandType))
							Expect(startCmd.Value).ToNot(BeNil())
							Expect(startCmd.Value).To(BeAssignableToTypeOf(&msg.StartCommand{}))

							close(done)
						})

						Context("server ACKs start command, it transitions to armed state, sleeps until start time, it transitions to running state, server and client are transmitting, ", func() {
							var runstateStatsResponder *openperfResponder
							var setRunstatePollCount chan int
							const runstatePollCount = 30 //This will be reduced by the channel above.
							var runstateStatsResponderCancel context.CancelFunc
							//client starts up a runstate poll command here.
							BeforeEach(func(done Done) {
								//ACK start command.
								peerRespIn <- &msg.Message{
									Type: msg.AckType,
								}

								Eventually(func() string {
									return fsm.State()
								}).Should(Equal("armed"))

								// Client sleeps here until start time.
								time.Sleep(time.Until(startCmd.Value.(*msg.StartCommand).StartTime))

								setRunstatePollCount = make(chan int)
								runstateStatsResponder = &openperfResponder{
									OPCmdIn:              opCmdOut,
									RunstatePollCount:    runstatePollCount,
									NewRunstatePollCount: setRunstatePollCount,
								}
								var responderCtx context.Context
								responderCtx, runstateStatsResponderCancel = context.WithCancel(context.Background())
								go handleOpenperfResponses(responderCtx, runstateStatsResponder)

								// Client switches to running state here.
								Eventually(func() string {
									return fsm.State()
								}).Should(Equal("running"))

								close(done)
							}, assertEpsilon.Seconds())

							// At this point either server or client will stop first.
							// Both are tested below for completeness

							Context("server sends notifications, server stops transmitting, ", func() {
								BeforeEach(func(done Done) {

									for i := 0; i < 4; i++ {
										peerNotifIn <- &msg.Message{
											Type: msg.StatsNotificationType,
											Value: &msg.RuntimeStats{
												Timestamp: uint64(time.Now().Unix()),
											},
										}
										time.Sleep(500 * time.Millisecond)
									}

									peerNotifIn <- &msg.Message{
										Type: msg.TransmitDoneType,
									}

									Expect(fsm.State()).To(Equal("running"))

									close(done)
								}, assertEpsilon.Seconds())

								Context("client continues to run, client stops, ", func() {
									BeforeEach(func(done Done) {
										//Sleep to simulate client running for a bit more.
										time.Sleep(1 * time.Second)

										setRunstatePollCount <- int(0)

										cmd := <-peerCmdOut
										Expect(cmd).NotTo(BeNil())
										Expect(cmd.Type).To(Equal(msg.TransmitDoneType))
										Expect(cmd.Value).To(BeNil())

										runstateStatsResponderCancel()

										Eventually(func() string {
											return fsm.State()
										}).Should(Equal("done"))

										cmd = <-peerCmdOut
										Expect(cmd).NotTo(BeNil())
										Expect(cmd.Type).To(Equal(msg.GetFinalStatsType))
										Expect(cmd.Value).To(BeNil())

										close(done)
									}, assertEpsilon.Seconds())

									Context("server sends final stats, ", func() {
										It("switches to the cleanup state", func(done Done) {
											peerRespIn <- &msg.Message{
												Type: msg.FinalStatsType,
												Value: &msg.FinalStats{
													TransmitFrames: uint64(420)},
											}

											Eventually(func() string {
												return fsm.State()
											}).Should(Equal("cleanup"))

											close(done)

										})

									})

								})
							})

							Context("server sends notifications, client stops transmitting, ", func() {
								var statsNotifDone chan struct{}
								BeforeEach(func(done Done) {
									statsNotifDone = make(chan struct{})
									go func() {
									Done:
										for {
											select {
											case <-statsNotifDone:
												break Done
											case <-time.After(1 * time.Second):
												peerNotifIn <- &msg.Message{
													Type: msg.StatsNotificationType,
													Value: &msg.RuntimeStats{
														Timestamp: uint64(time.Now().Unix()),
													},
												}
											}
										}
									}()

									setRunstatePollCount <- int(0)

									cmd := <-peerCmdOut
									Expect(cmd).NotTo(BeNil())
									Expect(cmd.Type).To(Equal(msg.TransmitDoneType))

									Expect(fsm.State()).To(Equal("running"))

									close(done)
								}, assertEpsilon.Seconds())

								Context("server continues to run, server stops, ", func() {
									BeforeEach(func(done Done) {
										//Sleep to simulate server running for a bit more.
										time.Sleep(1 * time.Second)

										close(statsNotifDone)
										peerNotifIn <- &msg.Message{
											Type: msg.TransmitDoneType,
										}

										cmd := <-peerCmdOut
										Expect(cmd).NotTo(BeNil())
										Expect(cmd.Type).To(Equal(msg.GetFinalStatsType))

										Expect(fsm.State()).To(Equal("done"))

										runstateStatsResponderCancel()

										close(done)
									}, assertEpsilon.Seconds())

									Context("server sends final stats, ", func() {
										It("switches to the cleanup state", func(done Done) {
											peerRespIn <- &msg.Message{
												Type: msg.FinalStatsType,
												Value: &msg.FinalStats{
													TransmitFrames: uint64(420)},
											}

											Eventually(func() string {
												return fsm.State()
											}).Should(Equal("cleanup"))

											close(done)

										}, assertEpsilon.Seconds())

									})
								})
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

func drainCleanupRequests(opCmd chan *openperf.Command) {

	for {
		select {
		case <-time.After(500 * time.Millisecond):
			return
		case cmd := <-opCmd:
			close(cmd.Done)
		}
	}
}

type openperfResponder struct {
	OPCmdIn chan *openperf.Command

	NewRunstatePollCount <-chan int

	// ErrorRunstate write an error here to have it appear in the runstate Request's Response field.
	ErrorRunstate <-chan struct{}
	// ErrorRxStats write an error here to have it appear in the stats Request's Response field.
	ErrorRxStats <-chan struct{}
	// ErrorTxStats write an error here to have it appear in the stats Request's Response field.
	ErrorTxStats <-chan struct{}

	//runstatePollCount how many times should the "port" object return running == true.
	RunstatePollCount int
}

func makeOPGeneratorResponse(running bool) *openperf.GetGeneratorResponse {
	return &openperf.GetGeneratorResponse{
		Running: running,
	}
}

func makeOPTxStatsResponse() *openperf.GetTxStatsResponse {

	return &openperf.GetTxStatsResponse{
		Timestamp: uint64(time.Now().Unix()),
	}
}

func makeOPRxStatsResponse() *openperf.GetRxStatsResponse {

	return &openperf.GetRxStatsResponse{
		Timestamp: uint64(time.Now().Unix()),
	}
}

func handleOpenperfResponses(ctx context.Context, responder *openperfResponder) {
	runstatePollsRemaining := responder.RunstatePollCount
	var rxStatsErr bool = false
	var txStatsErr bool = false
	var runstateErr bool = false

Done:
	for {
		select {
		case <-ctx.Done():
			break Done

		case req, ok := <-responder.OPCmdIn:
			if !ok {
				fmt.Println("something went really wrong!")
			}

			switch req.Request.(type) {
			case *openperf.GetGeneratorRequest:
				switch {
				case runstateErr == true:
					req.Response = errors.New("error reading generator from Openperf.")
					runstateErr = false
				case runstatePollsRemaining == 0:
					req.Response = makeOPGeneratorResponse(false)
				default:
					runstatePollsRemaining--
					req.Response = makeOPGeneratorResponse(true)
				}
				close(req.Done)

			case *openperf.GetTxStatsRequest:
				if txStatsErr == true {
					req.Response = errors.New("error reading tx stats from Openperf.")
					txStatsErr = false
				} else {
					req.Response = makeOPTxStatsResponse()
				}
				close(req.Done)

			case *openperf.GetRxStatsRequest:
				if rxStatsErr == true {
					req.Response = errors.New("error reading rx stats from Openperf.")
					rxStatsErr = false
				} else {
					req.Response = makeOPRxStatsResponse()
				}
				close(req.Done)
			}
		case <-responder.ErrorRunstate:
			runstateErr = true
		case <-responder.ErrorTxStats:
			txStatsErr = true
		case <-responder.ErrorRxStats:
			rxStatsErr = true
		case val := <-responder.NewRunstatePollCount:
			runstatePollsRemaining = val
		}
	}
}
