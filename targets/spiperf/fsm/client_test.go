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
	"github.com/onsi/gomega/types"

	. "github.com/Spirent/openperf/targets/spiperf/fsm"
	"github.com/Spirent/openperf/targets/spiperf/msg"
	"github.com/Spirent/openperf/targets/spiperf/openperf"
)

var _ = Describe("Client FSM,", func() {
	//Test-wide constants
	const assertEpsilon = time.Second * 15

	var (
		peerCmdOut         chan *msg.Message
		peerRespIn         chan *msg.Message
		peerNotifIn        chan *msg.Message
		opCmdOut           chan *openperf.Command
		dataStreamStatsOut chan *Stats
		fsmReturn          chan error
		fsm                *Client
		ctx                context.Context
		cancelFunc         context.CancelFunc
	)

	BeforeEach(func() {
		peerCmdOut = make(chan *msg.Message, 1)
		peerRespIn = make(chan *msg.Message, 1)
		peerNotifIn = make(chan *msg.Message, 1)
		opCmdOut = make(chan *openperf.Command)
		dataStreamStatsOut = make(chan *Stats, 1)
		fsmReturn = make(chan error)

		fsm = &Client{
			PeerCmdOut:            peerCmdOut,
			PeerRespIn:            peerRespIn,
			PeerNotifIn:           peerNotifIn,
			OpenperfCmdOut:        opCmdOut,
			DataStreamStatsOut:    dataStreamStatsOut,
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
				PeerProtocolVersion: msg.Version,
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
						PeerProtocolVersion: "9.90",
					},
				}

				ret := <-fsmReturn
				Expect(ret).To(BeAssignableToTypeOf(&PeerError{}))
				Expect(ret.Error()).ToNot(BeEmpty())
				Expect(peerCmdOut).To(BeClosed())

				close(done)
			})

		})

		Context("when peer returns an error, ", func() {
			It("terminates with an error", func(done Done) {
				peerRespIn <- &msg.Message{
					Type:  msg.ErrorType,
					Value: errors.New("server error"),
				}

				ret := <-fsmReturn
				Expect(ret).To(BeAssignableToTypeOf(&PeerError{}))
				Expect(ret.Error()).ToNot(BeEmpty())
				Expect(peerCmdOut).To(BeClosed())

				close(done)
			})
		})

		Context("when peer responds with an unexpected message type, ", func() {
			It("terminates with an error", func(done Done) {
				peerRespIn <- &msg.Message{
					Type: msg.GetConfigType,
				}

				ret := <-fsmReturn
				Expect(ret).To(BeAssignableToTypeOf(&PeerError{}))
				Expect(ret.Error()).ToNot(BeEmpty())
				Expect(peerCmdOut).To(BeClosed())

				close(done)
			})
		})

		Context("when peer responds with an incorrect message value, ", func() {
			It("terminates with an error", func(done Done) {
				peerRespIn <- &msg.Message{
					Type:  msg.HelloType,
					Value: &msg.StartCommand{},
				}

				ret := <-fsmReturn
				Expect(ret).To(BeAssignableToTypeOf(&PeerError{}))
				Expect(ret.Error()).ToNot(BeEmpty())
				Expect(peerCmdOut).To(BeClosed())

				close(done)
			})
		})

		Context("when peer does not respond to hello message, ", func() {
			It("terminates with an error", func(done Done) {
				ret := <-fsmReturn
				Expect(ret).To(BeAssignableToTypeOf(&TimeoutError{}))
				Expect(ret.Error()).ToNot(BeEmpty())
				Expect(peerCmdOut).To(BeClosed())
				close(done)
			})
		})

		Context("when peer response channel returns an error, ", func() {
			It("terminates with an error", func(done Done) {
				close(peerRespIn)
				ret := <-fsmReturn
				Expect(ret).To(BeAssignableToTypeOf(&PeerError{}))
				Expect(ret.Error()).ToNot(BeEmpty())
				Expect(peerCmdOut).To(BeClosed())
				close(done)
			})

		})

		Context("peer returns server parameters, it transitions to the configure state, ", func() {
			var cancelStatsHandler context.CancelFunc

			BeforeEach(func(done Done) {
				peerRespIn <- &msg.Message{
					Type: msg.HelloType,
					Value: &msg.Hello{
						PeerProtocolVersion: msg.Version,
					},
				}

				command := <-peerCmdOut
				Expect(command).NotTo(BeNil())
				Expect(command.Type).To(Equal(msg.GetConfigType))
				Expect(command.Value).To(BeNil())

				Expect(fsm.State()).To(Equal("configure"))

				// Start up handler for the StatsOut channel.
				var statsHandlerCtx context.Context
				statsHandlerCtx, cancelStatsHandler = context.WithCancel(context.Background())

				go func() {
					defer GinkgoRecover()
					handleStatsOutput(statsHandlerCtx, dataStreamStatsOut)
				}()

				close(done)
			})

			Context("when server returns an error, ", func() {
				It("terminates with an error", func(done Done) {
					peerRespIn <- &msg.Message{
						Type:  msg.ErrorType,
						Value: errors.New("server error"),
					}

					ret := <-fsmReturn
					Expect(ret).To(BeAssignableToTypeOf(&PeerError{}))
					Expect(ret.Error()).ToNot(BeEmpty())
					Expect(peerCmdOut).To(BeClosed())

					close(done)
				})
			})

			Context("when server returns invalid parameters, ", func() {
				It("terminates with an error", func(done Done) {
					peerRespIn <- &msg.Message{
						Type: msg.ServerParametersType,
						Value: &msg.ServerParametersResponse{
							OpenperfURL: ":http://localhost:9000",
						},
					}

					ret := <-fsmReturn
					Expect(ret).To(
						BeAssignableToTypeOf(&InvalidConfigurationError{}))
					Expect(ret.Error()).ToNot(BeEmpty())
					Expect(peerCmdOut).To(BeClosed())

					close(done)
				})
			})

			Context("when server does not respond to get param message", func() {
				It("terminates with an error", func(done Done) {
					ret := <-fsmReturn
					Expect(ret).To(BeAssignableToTypeOf(&TimeoutError{}))
					Expect(ret.Error()).ToNot(BeEmpty())
					Expect(peerCmdOut).To(BeClosed())

					close(done)
				})
			})

			Context("when server returns an incorrect message type, ", func() {
				It("terminates with an error", func(done Done) {
					peerRespIn <- &msg.Message{
						Type:  msg.HelloType,
						Value: &msg.Hello{},
					}

					ret := <-fsmReturn
					Expect(ret).To(
						BeAssignableToTypeOf(&PeerError{}))
					Expect(ret.Error()).ToNot(BeEmpty())
					Expect(peerCmdOut).To(BeClosed())

					close(done)
				})
			})

			Context("when server returns an incorrect Value, ", func() {
				It("terminates with an error", func(done Done) {
					peerRespIn <- &msg.Message{
						Type:  msg.ServerParametersType,
						Value: &msg.FinalStats{},
					}

					ret := <-fsmReturn
					Expect(ret).To(
						BeAssignableToTypeOf(&PeerError{}))
					Expect(ret.Error()).ToNot(BeEmpty())
					Expect(peerCmdOut).To(BeClosed())

					close(done)
				})
			})

			Context("when server disconnects unexpectedly instead of returning a value, ", func() {
				It("terminates with an error", func(done Done) {
					peerNotifIn <- &msg.Message{
						Type:  msg.PeerDisconnectLocalType,
						Value: &msg.PeerDisconnectLocalNotif{Err: errors.New("JSON encoder error")},
					}

					ret := <-fsmReturn
					Expect(ret).To(
						BeAssignableToTypeOf(&UnexpectedPeerDisconnectError{}))
					Expect(ret.Error()).ToNot(BeEmpty())
					Expect(ret.(*UnexpectedPeerDisconnectError).Local).To(BeTrue())
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

					fsm.TestConfiguration.UpstreamRateBps = 100
					fsm.TestConfiguration.DownstreamRateBps = 0

					peerRespIn <- &msg.Message{
						Type: msg.ServerParametersType,
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

				Context("when server does not ACK set configuration message, ", func() {
					It("exits with an error", func(done Done) {
						drainOpenperfCommands(opCmdOut, peerCmdOut)

						ret := <-fsmReturn
						Expect(ret).To(BeAssignableToTypeOf(&TimeoutError{}))
						Expect(ret.Error()).ToNot(BeEmpty())
						Expect(fsm.State()).To(Equal("cleanup"))
						close(done)
					}, assertEpsilon.Seconds())
				})

				Context("when server NACKs configuration from client, ", func() {
					It("exits with an error", func(done Done) {
						peerRespIn <- &msg.Message{
							Type:  msg.ErrorType,
							Value: errors.New("error with configuration"),
						}

						drainOpenperfCommands(opCmdOut, peerCmdOut)

						ret := <-fsmReturn
						Expect(ret).To(
							BeAssignableToTypeOf(&PeerError{}))
						Expect(ret.Error()).ToNot(BeEmpty())
						Expect(peerCmdOut).To(BeClosed())
						Expect(fsm.State()).To(Equal("cleanup"))

						close(done)
					}, assertEpsilon.Seconds())
				})

				Context("when server sends unexpected message type, ", func() {
					It("exits with an error", func(done Done) {
						peerRespIn <- &msg.Message{
							Type: msg.GetConfigType,
						}

						drainOpenperfCommands(opCmdOut, peerCmdOut)

						ret := <-fsmReturn
						Expect(ret).To(
							BeAssignableToTypeOf(&PeerError{}))
						Expect(ret.Error()).ToNot(BeEmpty())
						Expect(peerCmdOut).To(BeClosed())
						Expect(fsm.State()).To(Equal("cleanup"))

						close(done)
					})

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

					Context("when openperf get time command times out, ", func() {
						It("exits with an error", func(done Done) {
							// Openperf requests will never instantaneously time out.
							// Sleep here for a little while to more accurately reflect reality.
							time.Sleep(10 * time.Millisecond)
							openperfGetTimeCmd.Response = context.DeadlineExceeded
							close(openperfGetTimeCmd.Done)

							drainOpenperfCommands(opCmdOut, peerCmdOut)

							ret := <-fsmReturn
							Expect(ret).To(BeAssignableToTypeOf(&OpenperfError{}))
							Expect(peerCmdOut).To(BeClosed())
							Expect(fsm.State()).To(Equal("cleanup"))

							close(done)

						})

					})

					Context("when openperf get time command returns an error, ", func() {
						It("exits with an error", func(done Done) {
							openperfGetTimeCmd.Response = errors.New("error getting command from Openperf")
							close(openperfGetTimeCmd.Done)

							drainOpenperfCommands(opCmdOut, peerCmdOut)

							ret := <-fsmReturn
							Expect(ret).To(BeAssignableToTypeOf(&OpenperfError{}))
							Expect(peerCmdOut).To(BeClosed())
							Expect(fsm.State()).To(Equal("cleanup"))

							close(done)
						})
					})

					Context("openperf returns the current time, ", func() {
						var startCmd *msg.Message
						BeforeEach(func(done Done) {
							openperfGetTimeCmd.Response = &openperf.GetTimeResponse{Time: conv.DateTime(strfmt.DateTime(time.Now()))}
							close(openperfGetTimeCmd.Done)

							startCmd = <-peerCmdOut

							Expect(startCmd).ToNot(BeNil())
							Expect(startCmd.Type).To(Equal(msg.StartCommandType))
							Expect(startCmd.Value).ToNot(BeNil())
							Expect(startCmd.Value).To(BeAssignableToTypeOf(&msg.StartCommand{}))
							startTime, err := time.Parse(TimeFormatString, startCmd.Value.(*msg.StartCommand).StartTime)
							Expect(err).To(BeNil())
							Expect(startTime).To(BeTemporally("~", time.Now().Add(fsm.StartDelay), time.Millisecond*1000))

							close(done)
						})

						Context("when server NACKs start command, ", func() {
							It("exits with an error", func(done Done) {
								peerRespIn <- &msg.Message{
									Type:  msg.ErrorType,
									Value: errors.New("error with configuration"),
								}

								drainOpenperfCommands(opCmdOut, peerCmdOut)

								ret := <-fsmReturn
								Expect(ret).To(
									BeAssignableToTypeOf(&PeerError{}))
								Expect(ret.Error()).ToNot(BeEmpty())
								Expect(peerCmdOut).To(BeClosed())
								Expect(fsm.State()).To(Equal("cleanup"))

								close(done)

							})
						})

						Context("when server does not respond to start command, ", func() {
							It("exits with an error", func(done Done) {

								drainOpenperfCommands(opCmdOut, peerCmdOut)

								ret := <-fsmReturn
								Expect(ret).To(
									BeAssignableToTypeOf(&TimeoutError{}))
								Expect(ret.Error()).ToNot(BeEmpty())
								Expect(peerCmdOut).To(BeClosed())
								Expect(fsm.State()).To(Equal("cleanup"))

								close(done)

							})
						})

						Context("when server sends unexpected response to start command, ", func() {
							It("exits with an error", func(done Done) {
								peerRespIn <- &msg.Message{
									Type:  msg.HelloType,
									Value: &msg.Hello{},
								}

								drainOpenperfCommands(opCmdOut, peerCmdOut)

								ret := <-fsmReturn
								Expect(ret).To(
									BeAssignableToTypeOf(&PeerError{}))
								Expect(ret.Error()).ToNot(BeEmpty())
								Expect(peerCmdOut).To(BeClosed())
								Expect(fsm.State()).To(Equal("cleanup"))

								close(done)

							})
						})

						Context("server ACKs start command, when peer disconnects during armed state, ", func() {
							It("exits with an error", func(done Done) {
								//ACK start command.
								peerRespIn <- &msg.Message{
									Type: msg.AckType,
								}

								peerNotifIn <- &msg.Message{
									Type:  msg.PeerDisconnectRemoteType,
									Value: &msg.PeerDisconnectRemoteNotif{Err: "aborting test."},
								}

								drainOpenperfCommands(opCmdOut, peerCmdOut)

								ret := <-fsmReturn
								Expect(ret).To(
									BeAssignableToTypeOf(&UnexpectedPeerDisconnectError{}))
								Expect(ret.Error()).ToNot(BeEmpty())
								Expect(ret.(*UnexpectedPeerDisconnectError).Local).To(BeFalse())
								Expect(peerCmdOut).To(BeClosed())
								Expect(fsm.State()).To(Equal("cleanup"))

								close(done)
							}, assertEpsilon.Seconds())

						})

						Context("server ACKs start command, when local peer interface errors during armed state, ", func() {
							It("exits with an error", func(done Done) {
								//ACK start command.
								peerRespIn <- &msg.Message{
									Type: msg.AckType,
								}

								peerNotifIn <- &msg.Message{
									Type:  msg.PeerDisconnectLocalType,
									Value: &msg.PeerDisconnectLocalNotif{Err: errors.New("unexpected local peer communcation error")},
								}

								drainOpenperfCommands(opCmdOut, peerCmdOut)

								ret := <-fsmReturn
								Expect(ret).To(
									BeAssignableToTypeOf(&UnexpectedPeerDisconnectError{}))
								Expect(ret.Error()).ToNot(BeEmpty())
								Expect(ret.(*UnexpectedPeerDisconnectError).Local).To(BeTrue())
								Expect(peerCmdOut).To(BeClosed())
								Expect(fsm.State()).To(Equal("cleanup"))

								close(done)
							}, assertEpsilon.Seconds())

						})

						Context("server ACKs start command, it transitions to armed state, sleeps until start time, it transitions to runningTx state, client is transmitting, ", func() {

							// At this point the test is running and we need to respond to
							// async requests from the client. Prior to this the Openperf requests
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
								//ACK start command.
								peerRespIn <- &msg.Message{
									Type: msg.AckType,
								}

								Eventually(func() string {
									return fsm.State()
								}).Should(Equal("armed"))

								startTime, err := time.Parse(TimeFormatString, startCmd.Value.(*msg.StartCommand).StartTime)
								Expect(err).To(BeNil())
								// Client sleeps here until start time.
								time.Sleep(time.Until(startTime))

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
										Value: errors.New("error with rx stats"),
									}

									ret := <-fsmReturn
									Expect(ret).To(BeAssignableToTypeOf(&PeerError{}))
									Expect(peerCmdOut).To(BeClosed())
									Expect(fsm.State()).To(Equal("cleanup"))

									close(done)
								}, assertEpsilon.Seconds())
							})

							Context("when server disconnects while test is running, ", func() {
								It("exits with an error", func(done Done) {
									peerNotifIn <- &msg.Message{
										Type:  msg.PeerDisconnectRemoteType,
										Value: &msg.PeerDisconnectRemoteNotif{Err: "aborting test."},
									}

									ret := <-fsmReturn
									Expect(ret).To(
										BeAssignableToTypeOf(&UnexpectedPeerDisconnectError{}))
									Expect(ret.Error()).ToNot(BeEmpty())
									Expect(ret.(*UnexpectedPeerDisconnectError).Local).To(BeFalse())
									Expect(peerCmdOut).To(BeClosed())
									Expect(fsm.State()).To(Equal("cleanup"))

									close(done)

								}, assertEpsilon.Seconds())

							})

							Context("when openperf returns an error while polling generator, ", func() {
								It("exits with an error", func(done Done) {
									runstateResponderError <- struct{}{}

									val := <-fsmReturn
									Expect(val).To(BeAssignableToTypeOf(&OpenperfError{}))
									Expect(peerCmdOut).To(BeClosed())
									Expect(fsm.State()).To(Equal("cleanup"))

									close(done)

								}, assertEpsilon.Seconds())

							})

							Context("when openperf returns an error while polling Tx stats, ", func() {
								It("exits with an error", func(done Done) {
									txStatsResponderError <- struct{}{}

									val := <-fsmReturn
									Expect(val).To(BeAssignableToTypeOf(&OpenperfError{}))
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
													Value: &msg.DataStreamStats{
														RxStats: &openperf.GetRxStatsResponse{
															Timestamp: time.Now().Format(TimeFormatString),
														},
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
											Value: errors.New("error gathering final stats")}

										ret := <-fsmReturn
										Expect(ret).To(BeAssignableToTypeOf(&PeerError{}))
										Expect(ret.Error()).ToNot(BeEmpty())
										Expect(peerCmdOut).To(BeClosed())
										Expect(fsm.State()).To(Equal("cleanup"))

										close(done)
									})

								})

								Context("when server does not send any final stats, ", func() {
									It("exits with an error", func(done Done) {

										ret := <-fsmReturn
										Expect(ret).To(
											BeAssignableToTypeOf(&TimeoutError{}))
										Expect(ret.Error()).ToNot(BeEmpty())
										Expect(peerCmdOut).To(BeClosed())
										Expect(fsm.State()).To(Equal("cleanup"))

										close(done)
									})

								})

								Context("when server sends an unexpected message instead of final stats, ", func() {
									It("exits with an error", func(done Done) {
										peerRespIn <- &msg.Message{
											Type:  msg.HelloType,
											Value: &msg.Hello{},
										}

										ret := <-fsmReturn
										Expect(ret).To(
											BeAssignableToTypeOf(&PeerError{}))
										Expect(ret.Error()).ToNot(BeEmpty())
										Expect(peerCmdOut).To(BeClosed())
										Expect(fsm.State()).To(Equal("cleanup"))

										close(done)
									})

								})

								Context("server sends final stats, it switches to the cleanup state, when Openperf errors with cleanup, ", func() {
									It("exits with an error", func(done Done) {

										genDoneResponderError <- struct{}{}

										peerRespIn <- &msg.Message{
											Type: msg.FinalStatsType,
											Value: &msg.FinalStats{
												RxStats: &openperf.GetRxStatsResponse{Timestamp: time.Now().Format(TimeFormatString)},
											},
										}

										Eventually(func() string {
											return fsm.State()
										}).Should(Equal("cleanup"))

										ret := <-fsmReturn
										Expect(ret).To(BeAssignableToTypeOf(&OpenperfError{}))
										Expect(peerCmdOut).To(BeClosed())
										Expect(fsm.State()).To(Equal("cleanup"))

										close(done)

									})
								})

								Context("server sends final stats, it switches to the cleanup state, when it cleans up, ", func() {
									It("returns without error.", func(done Done) {
										peerRespIn <- &msg.Message{
											Type: msg.FinalStatsType,
											Value: &msg.FinalStats{
												RxStats: &openperf.GetRxStatsResponse{Timestamp: time.Now().Format(TimeFormatString)},
											},
										}

										res := <-fsmReturn
										Expect(res).To(BeNil())

										Expect(fsm.State()).To(Equal("cleanup"))

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

			Context("server returns valid parameters, test has server -> client traffic, ", func() {
				BeforeEach(func() {

					fsm.TestConfiguration.UpstreamRateBps = 0
					fsm.TestConfiguration.DownstreamRateBps = 100

					peerRespIn <- &msg.Message{
						Type: msg.ServerParametersType,
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
							openperfGetTimeCmd.Response = &openperf.GetTimeResponse{Time: conv.DateTime(strfmt.DateTime(time.Now()))}
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

								startTime, err := time.Parse(TimeFormatString, startCmd.Value.(*msg.StartCommand).StartTime)
								Expect(err).To(BeNil())
								// Client sleeps here until start time.
								time.Sleep(time.Until(startTime))

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
								// Client switches to running state here.

								close(done)
							}, assertEpsilon.Seconds())

							Context("when openperf returns an error while polling Rx stats, ", func() {
								It("exits with an error", func(done Done) {
									rxStatsResponderError <- struct{}{}

									// Wait for the system to get into the cleanup state, else
									// there could be in-flight poll requests.
									Eventually(func() string { return fsm.State() }, 2*time.Second).Should(Equal("cleanup"))

									val := <-fsmReturn
									Expect(val).To(BeAssignableToTypeOf(&OpenperfError{}))
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
											Value: &msg.DataStreamStats{
												TxStats: &openperf.GetTxStatsResponse{
													Timestamp: time.Now().Format(TimeFormatString),
												},
											},
										}
										time.Sleep(500 * time.Millisecond)
									}

									peerNotifIn <- &msg.Message{
										Type: msg.TransmitDoneType,
									}

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
												TxStats: &openperf.GetTxStatsResponse{Timestamp: time.Now().Format(TimeFormatString)},
											},
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

					fsm.TestConfiguration.UpstreamRateBps = 100
					fsm.TestConfiguration.DownstreamRateBps = 100

					peerRespIn <- &msg.Message{
						Type: msg.ServerParametersType,
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
							openperfGetTimeCmd.Response = &openperf.GetTimeResponse{Time: conv.DateTime(strfmt.DateTime(time.Now()))}
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

								startTime, err := time.Parse(TimeFormatString, startCmd.Value.(*msg.StartCommand).StartTime)
								Expect(err).To(BeNil())
								// Client sleeps here until start time.
								time.Sleep(time.Until(startTime))

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
											Value: &msg.DataStreamStats{
												TxStats: &openperf.GetTxStatsResponse{
													Timestamp: time.Now().Format(TimeFormatString),
												},
												RxStats: &openperf.GetRxStatsResponse{
													Timestamp: time.Now().Format(TimeFormatString),
												},
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
													TxStats: &openperf.GetTxStatsResponse{Timestamp: time.Now().Format(TimeFormatString)},
													RxStats: &openperf.GetRxStatsResponse{Timestamp: time.Now().Format(TimeFormatString)},
												},
											}

											Eventually(func() string {
												return fsm.State()
											}).Should(Equal("cleanup"))

											runstateStatsResponderCancel()

											close(done)

										}, assertEpsilon.Seconds())

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
													Value: &msg.DataStreamStats{
														RxStats: &openperf.GetRxStatsResponse{
															Timestamp: time.Now().Format(TimeFormatString),
														},
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

										close(done)
									}, assertEpsilon.Seconds())

									Context("server sends final stats, ", func() {
										It("switches to the cleanup state", func(done Done) {
											peerRespIn <- &msg.Message{
												Type: msg.FinalStatsType,
												Value: &msg.FinalStats{
													TxStats: &openperf.GetTxStatsResponse{Timestamp: time.Now().Format(TimeFormatString)},
													RxStats: &openperf.GetRxStatsResponse{Timestamp: time.Now().Format(TimeFormatString)},
												},
											}

											Eventually(func() string {
												return fsm.State()
											}).Should(Equal("cleanup"))

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

			AfterEach(func(done Done) {
				cancelStatsHandler()
				close(done)
			}, assertEpsilon.Seconds())
		})
	})
	AfterEach(func() {
		cancelFunc()
	})
})

// Unit test utilities

func drainOpenperfCommands(opCmd chan *openperf.Command, peerCmdOut chan *msg.Message) {

	for {
		select {
		// FSM closes peerCmdOut when it terminates. Use this to know if more Openperf requests are coming.
		case <-peerCmdOut:
			return
		case cmd := <-opCmd:
			close(cmd.Done)
		}
	}
}

type openperfResponder struct {
	OPCmdIn chan *openperf.Command

	// NewRunstatePollCount write the number of runstate requests that will have running set to true.
	NewRunstatePollCount <-chan int

	// ErrorRunstate write and the next runstate request will respond with an error.
	ErrorRunstate <-chan struct{}
	// ErrorRxStats write and the next Rx stats request will respond with an error.
	ErrorRxStats <-chan struct{}
	// ErrorTxStats write and the next Tx stats request will respond with an error.
	ErrorTxStats <-chan struct{}
	// ErrorDeleteGen write and the next Delete Generator request will respond with an error.
	ErrorDeleteGen <-chan struct{}

	// RunstatePollCount how many times should the "port" object return running == true.
	RunstatePollCount int
}

func makeOPGeneratorResponse(running bool) *openperf.GetGeneratorResponse {
	return &openperf.GetGeneratorResponse{
		Running: running,
	}
}

func makeOPTxStatsResponse() *openperf.GetTxStatsResponse {

	return &openperf.GetTxStatsResponse{
		Timestamp: time.Now().Format(TimeFormatString),
	}
}

func makeOPRxStatsResponse() *openperf.GetRxStatsResponse {

	return &openperf.GetRxStatsResponse{
		Timestamp: time.Now().Format(TimeFormatString),
	}
}

func handleOpenperfResponses(ctx context.Context, responder *openperfResponder) {
	runstatePollsRemaining := responder.RunstatePollCount
	var (
		rxStatsErr   bool = false
		txStatsErr   bool = false
		runstateErr  bool = false
		deleteGenErr bool = false
	)

Done:
	for {
		select {
		case <-ctx.Done():
			break Done

		case req, ok := <-responder.OPCmdIn:
			if !ok {
				return
			}

			Expect(req).ToNot(BeNil())
			Expect(req.Ctx).ToNot(BeNil())
			Expect(req.Done).ToNot(BeNil())

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

			case *openperf.GetTxStatsRequest:
				if txStatsErr {
					req.Response = errors.New("error reading tx stats from Openperf.")
					txStatsErr = false
				} else {
					req.Response = makeOPTxStatsResponse()
				}

			case *openperf.GetRxStatsRequest:
				if rxStatsErr {
					req.Response = errors.New("error reading rx stats from Openperf.")
					rxStatsErr = false
				} else {
					req.Response = makeOPRxStatsResponse()
				}

			case *openperf.DeleteGeneratorRequest:
				if deleteGenErr {
					req.Response = errors.New("error deleting generator resource.")
					deleteGenErr = false
				}

			}
			close(req.Done)

		case <-responder.ErrorRunstate:
			runstateErr = true
		case <-responder.ErrorTxStats:
			txStatsErr = true
		case <-responder.ErrorRxStats:
			rxStatsErr = true
		case val := <-responder.NewRunstatePollCount:
			runstatePollsRemaining = val
		case <-responder.ErrorDeleteGen:
			deleteGenErr = true
		}
	}
}

func handleStatsOutput(ctx context.Context, stats chan *Stats) {
	for {
		select {
		case <-ctx.Done():
			return
		case message := <-stats:
			Expect(message).ToNot(BeNil())
			Expect(message.Kind).To(BeStatsKind())

			switch message.Kind {
			case UpstreamTxKind:
				Expect(message.Values).To(BeAssignableToTypeOf(&openperf.GetTxStatsResponse{}))
			case UpstreamRxKind:
				Expect(message.Values).To(BeAssignableToTypeOf(&openperf.GetRxStatsResponse{}))
			case DownstreamTxKind:
				Expect(message.Values).To(BeAssignableToTypeOf(&openperf.GetTxStatsResponse{}))
			case DownstreamRxKind:
				Expect(message.Values).To(BeAssignableToTypeOf(&openperf.GetRxStatsResponse{}))

			}
		}
	}
}

func BeStatsKind() types.GomegaMatcher {
	return &statsKindMatcher{}
}

type statsKindMatcher struct {
}

func (matcher *statsKindMatcher) Match(actual interface{}) (success bool, err error) {

	val, ok := actual.(StatsKind)
	if !ok {
		return false, fmt.Errorf("BeStatsKind matcher expects a StatsKind, got %T", actual)
	}

	return val == UpstreamTxKind || val == UpstreamRxKind || val == DownstreamTxKind || val == DownstreamRxKind, nil

}

func (matcher *statsKindMatcher) FailureMessage(actual interface{}) (message string) {
	return fmt.Sprintf("Expected %s to be one of %s, %s, %s, or %s\n", actual, UpstreamTxKind, UpstreamRxKind, DownstreamTxKind, DownstreamRxKind)
}

func (matcher *statsKindMatcher) NegatedFailureMessage(actual interface{}) (message string) {
	return fmt.Sprintf("Expected %s to not be one of %s, %s, %s, or %s\n", actual, UpstreamTxKind, UpstreamRxKind, DownstreamTxKind, DownstreamRxKind)
}
