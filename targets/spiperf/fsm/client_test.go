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

const assertEpsilon = time.Second * 15

var _ = Describe("Client FSM,", func() {
	//Test-wide constants
	//const ()

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
		peerCmdOut = make(chan *msg.Message)
		peerRespIn = make(chan *msg.Message)
		peerNotifIn = make(chan *msg.Message)
		opCmdOut = make(chan *openperf.Command)
		fsmReturn = make(chan error)

		fsm = &Client{
			PeerCmdOut:      peerCmdOut,
			PeerRespIn:      peerRespIn,
			PeerNotifIn:     peerNotifIn,
			OpenperfCmdOut:  opCmdOut,
			PeerTimeout:     500 * time.Millisecond,
			OpenperfTimeout: 500 * time.Millisecond,
			StartDelay:      1 * time.Second,
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
			//FIXME: make local object here.			hello:=
			Expect(command.Value.(*msg.Hello).Version).ToNot(BeEmpty())

			Expect(fsm.State()).To(Equal("connect"))
			close(done)
		}) //FIXME add timeout here.

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
						Version: "1.0",
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
			//  server -> client path.
			// This needs to be done prior to returning the server parameters since
			//  the client builds the configuration immediately after getting this response.

			Context("server returns valid parameters, test has client -> server traffic, ", func() {
				//FIXME: declare server configuration msg variable here?
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
					Expect(command.Value).To(BeAssignableToTypeOf(&msg.ServerConfiguration{}))
					//FIXME inspect fields of message to verify correctness.
				})

				Context("when server NACKs configuration from client, ", func() {
					It("exits with an error", func(done Done) {
						peerRespIn <- &msg.Message{
							Type:  msg.ErrorType,
							Value: "error with configuration",
						}

						Eventually(fsmReturn).Should(Receive(
							BeAssignableToTypeOf(&InvalidConfigurationError{})))
						Expect(peerCmdOut).To(BeClosed())

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

						//Openperf get time command
						openperfGetTimeCmd = <-opCmdOut
					})

					Context("when openperf command times out, ", func() {
						It("exits with an error", func(done Done) {
							// Openperf requests will never instantaneously time out.
							// Sleep here for a little while to more accurately reflect reality.
							time.Sleep(10 * time.Millisecond)
							openperfGetTimeCmd.Response = context.DeadlineExceeded
							close(openperfGetTimeCmd.Done)

							Eventually(fsmReturn).Should(Receive(
								BeAssignableToTypeOf(&InternalError{})))
							Expect(peerCmdOut).To(BeClosed())

							close(done)

						})

					})

					Context("when openperf controller returns an error, ", func() {
						It("exits with an error", func(done Done) {
							openperfGetTimeCmd.Response = errors.New("error getting command from Openperf")
							close(openperfGetTimeCmd.Done)

							Eventually(fsmReturn).Should(
								Receive(BeAssignableToTypeOf(&InternalError{})))
							Expect(peerCmdOut).To(BeClosed())

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
							Expect(startCmd).ToNot(BeNil())
							Expect(startCmd.Type).To(Equal(msg.StartCommandType))
							Expect(startCmd.Value).ToNot(BeNil())
							Expect(startCmd.Value).To(BeAssignableToTypeOf(&msg.StartCommand{}))
							//Expect(startCmd.Value.(*msg.StartCommand).StartTime).To(BeNumerically(">", 0))
							//require start command time to be roughly now + StartDelay
							//also other stuff.

							close(done)
						})

						Context("when server NACKs start command, ", func() {
							It("exits with an error", func(done Done) {
								peerRespIn <- &msg.Message{
									Type:  msg.ErrorType,
									Value: "error with configuration",
								}

								Eventually(fsmReturn).Should(Receive(
									BeAssignableToTypeOf(&InvalidConfigurationError{})))
								Expect(peerCmdOut).To(BeClosed())

								close(done)

							})
						})

						Context("server ACKs start command, it transitions to armed state, sleeps until start time, it transitions to runningTx state, client is transmitting, ", func() {
							//var opPollStatsReq *openperf.Request
							// killPollStats write an error to have polling command
							// canceled and the error written into the Result field.
							//var killPollStats chan error
							//var killPollStats context.CancelFunc
							//var opPollRunstateReq *openperf.RequestRepeater
							// killPollRunstate write an error to have polling command
							// canceled and the error written into the Result field.
							//var killPollRunstate chan error
							//var killPollRunstate context.CancelFunc

							var runstateStatsHandler *runstateStatsPollHandler
							var statsHandlerError chan struct{}
							var runstateHandlerError chan struct{}
							const runstatePollCount = 3
							var runstateHandlerCancel context.CancelFunc
							//client starts up a runstate poll command here.
							BeforeEach(func(done Done) {
								//fmt.Println("going to send ack to client.")
								//ACK start command.
								peerRespIn <- &msg.Message{
									Type: msg.AckType,
								}

								//fmt.Println("going to sleep till start time.")
								//pretty.Println(startCmd)
								// Client sleeps here until start time.
								fmt.Printf("server sleeping for %s\n", time.Until(startCmd.Value.(*msg.StartCommand).StartTime).String())
								time.Sleep(time.Until(startCmd.Value.(*msg.StartCommand).StartTime))

								statsHandlerError = make(chan struct{})
								runstateHandlerError = make(chan struct{})
								runstateStatsHandler = &runstateStatsPollHandler{
									OPCmdIn:           opCmdOut,
									ErrorRunstate:     runstateHandlerError,
									ErrorStats:        statsHandlerError,
									RunstatePollCount: runstatePollCount,
								}
								var handlerCtx context.Context
								handlerCtx, runstateHandlerCancel = context.WithCancel(context.Background())
								go handleRunstateStatsResponses(handlerCtx, runstateStatsHandler)
								// opPollStatsReq = <-opCmdOut
								// Expect(opPollStatsReq).NotTo(BeNil())
								// Expect(opPollStatsReq.Done).NotTo(BeNil())
								// Expect(opPollStatsReq.Command).NotTo(BeNil())
								// Expect(opPollStatsReq.Response).To(BeNil())
								// Expect(opPollStatsReq.Command).To(BeAssignableToTypeOf(&openperf.PollStatsCommand{}))
								//FIXME: add additional verification here.

								//killPollStats = make(chan error)
								// var pollStatsCtx context.Context
								// pollStatsCtx, killPollStats = context.WithCancel(context.Background())

								// opPollRunstateReq = <-opCmdOut
								// Expect(opPollRunstateReq).NotTo(BeNil())
								// Expect(opPollRunstateReq.Done).NotTo(BeNil())
								// Expect(opPollRunstateReq.Command).NotTo(BeNil())
								// Expect(opPollRunstateReq.Response).To(BeNil())
								// Expect(opPollRunstateReq.Command).To(BeAssignableToTypeOf(&openperf.PollRunstateCommand{}))
								//FIXME: add additional verifications here.

								//killPollRunstate = make(chan error)
								//var pollRunstateCtx context.Context
								//pollRunstateCtx, killPollRunstate = context.WithCancel(context.Background())

								// Go routine to simulate Openperf poll stats handler.
								// go func(killPoll context.Context, perReq *openperf.PeriodicRequest) {
								// 	req := perReq.Request.(*openperf.Request)
								// Done:
								// 	for {
								// 		select {
								// 		case <-killPoll.Done():
								// 			//req.Result = err
								// 			//req.Result = killPoll.Err()
								// 			break Done
								// 		case <-time.After(cmd.Interval):
								// 			//Send a stats notification to Output chan.
								// 			perReq.Responses <- &openperf.GetStatsResponse{
								// 				Timestamp: uint64(time.Now().Unix())}
								// 		case <-req.Ctx.Done():
								// 			//Done polling stats.
								// 			break Done
								// 		}
								// 	}

								// 	close(perReq.Responses)
								// 	close(req.Done)
								// }(pollStatsCtx, opPollStatsReq)

								// Go routine to simulate Openperf poll runstate handler.
								// go func(killPoll context.Context, perReq *openperf.PeriodicRequest) {
								// 	req := perReq.Request.(*openperf.Request)
								// Done:
								// 	for {
								// 		select {
								// 		case <-killPoll.Done():
								// 			//req.Result = err
								// 			//req.Result = killPoll.Err()
								// 			break Done
								// 		case <-time.After(cmd.Interval):
								// 			//Send a stats notification to Output chan.
								// 			perReq.Responses <- &openperf.GetPortResponse{}
								// 		case <-perReq.Ctx.Done():
								// 			//Done polling stats.
								// 			break Done
								// 		}
								// 	}

								// 	close(cmd.Output)
								// 	close(req.Done)
								// }(pollRunstateCtx, opPollRunstateReq)

								// Go routine to simulate Openperf poll runstate handler.
								// go func(killPoll context.Context, req *openperf.Request) {

								// 	select {
								// 	case err := <-killChan:
								// 		req.Result = err
								// 	case <-time.After(1 * time.Second): //FIXME!
								// 		req.Result = nil
								// 	}

								// 	close(req.Done)
								// }(killPollRunstate, opPollRunstateReq)

								// Client switches to runningTx state here.

								//fmt.Println("going to wait for runningTx state.")
								Eventually(func() string {
									return fsm.State()
								}).Should(Equal("runningTx"))

								close(done)
							}, assertEpsilon.Seconds())

							Context("when server sends an error on notification channel, ", func() {
								It("exits with an error", func(done Done) {
									//fmt.Println("going to send error stats notification.")
									peerNotifIn <- &msg.Message{
										Type:  msg.ErrorType,
										Value: "error with rx stats",
									}

									Eventually(fsmReturn).Should(Receive(
										BeAssignableToTypeOf(&InternalError{})))
									Expect(peerCmdOut).To(BeClosed())

									close(done)
								})
							})

							Context("when openperf returns an error while polling runstate, ", func() {
								It("exits with an error", func(done Done) {
									runstateHandlerError <- struct{}{}

									val := <-fsmReturn
									Expect(val).To(BeAssignableToTypeOf(&InternalError{}))
									Expect(peerCmdOut).To(BeClosed())

									close(done)

								}, assertEpsilon.Seconds())

							})

							Context("when server sends an unexpected stop notification, ", func() {
								It("exits with an error", func(done Done) {
									peerNotifIn <- &msg.Message{
										Type: msg.StopNotificationType,
									}

									Eventually(fsmReturn).Should(Receive(
										BeAssignableToTypeOf(&MessagingError{})))
									Expect(peerCmdOut).To(BeClosed())

									close(done)
								})
							})

							Context("server sends notifications, test completes, it switches to the done state, ", func() {
								BeforeEach(func(done Done) {
								Finished:
									for {
										select {
										case <-time.After(1 * time.Second):
											peerNotifIn <- &msg.Message{
												Type: msg.StatsNotificationType,
												Value: &msg.RuntimeStats{
													Timestamp: uint64(time.Now().Unix()),
												},
											}

										case cmd := <-peerCmdOut:
											Expect(cmd).NotTo(BeNil())
											Expect(cmd.Type).To(Equal(msg.TransmitDoneType))

											break Finished
										}
									}

									runstateHandlerCancel()

									Eventually(func() string {
										return fsm.State()
									}).Should(Equal("done"))

									cmd := <-peerCmdOut
									Expect(cmd).NotTo(BeNil())
									Expect(cmd.Type).To(Equal(msg.GetFinalStatsType))

									close(done)
								}, assertEpsilon.Seconds())

								Context("when server sends an error instead of final stats, ", func() {
									It("exits with an error", func() {
										peerRespIn <- &msg.Message{
											Type:  msg.ErrorType,
											Value: "error gathering final stats"}
									})

								})

								Context("server sends final stats, it switches to the disconnect state, client sends cleanup command, it switches to the cleanup state, ", func() {
									BeforeEach(func(done Done) {
										peerRespIn <- &msg.Message{
											Type: msg.FinalStatsType,
											Value: &msg.FinalStats{
												TransmitFrames: uint64(420)},
										}

										Eventually(func() string {
											return fsm.State()
										}).Should(Equal("disconnect"))

										cmd := <-peerCmdOut
										Expect(cmd).NotTo(BeNil())
										Expect(cmd.Type).To(Equal(msg.CleanupType))

										Eventually(func() string {
											return fsm.State()
										}).Should(Equal("cleanup"))

										close(done)
									})

									Context("when Openperf errors with cleanup, ", func() {
										It("exits with an error", func() {

										})

									})

									Context("it cleans up, ", func() {
										It("returns without error.", func(done Done) {
											req := <-opCmdOut
											Expect(req).NotTo(BeNil())
											Expect(req.Request).To(BeAssignableToTypeOf(&openperf.DeleteGeneratorRequest{}))
											close(req.Done)

											fmt.Println("reading final fsm state")
											//Eventually(fsmReturn).Should(Receive(BeNil()))
											res := <-fsmReturn
											Expect(res).To(BeNil())

											close(done)
										}, assertEpsilon.Seconds())
									})
								})
							})

							AfterEach(func() {
								runstateHandlerCancel()
							})

						})
					})
				})
			})

			Context("server returns valid parameters, test has server -> client traffic, ", func() {
				//FIXME: declare server configuration msg variable here?
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
					Expect(command.Value).To(BeAssignableToTypeOf(&msg.ServerConfiguration{}))
					//FIXME inspect fields of message to verify correctness.
				})

				Context("server ACKs configuration from client, it transitions to ready state, ", func() {
					var openperfGetTimeCmd *openperf.Command
					BeforeEach(func() {
						//Ack setconfig
						peerRespIn <- &msg.Message{
							Type: msg.AckType,
						}

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
							//Expect(startCmd.Value.(*msg.StartCommand).StartTime).To(BeNumerically(">", 0))
							//require start command time to be roughly now + StartDelay
							//also other stuff.

							close(done)
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

//AfterEach(func() {
//Eventually(fsmReturn).Should(Receive(BeNil()))
//})

//})

type runstateStatsPollHandler struct {
	OPCmdIn chan *openperf.Command

	// ErrorRunstate write an error here to have it appear in the runstate Request's Response field.
	ErrorRunstate chan struct{}
	// ErrorStats write an error here to have it appear in the stats Request's Response field.
	ErrorStats chan struct{}

	//runstatePollCount how many times should the "port" object return running == true.
	RunstatePollCount int
}

func makeOPGeneratorResponse(running bool) *openperf.GetGeneratorResponse {
	return &openperf.GetGeneratorResponse{
		Running: running,
	}
}

func makeOPStatsResponse() *openperf.GetStatsResponse {

	return &openperf.GetStatsResponse{
		Timestamp: uint64(time.Now().Unix()),
	}
}

func handleRunstateStatsResponses(ctx context.Context, handler *runstateStatsPollHandler) {
	runstatePollsRemaining := handler.RunstatePollCount
	var statsErr bool = false
	var runstateErr bool = false
Done:
	for {
		select {
		case <-ctx.Done():
			break Done

		case req, ok := <-handler.OPCmdIn:
			fmt.Println("got command in handler")
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

			case *openperf.GetStatsRequest:
				if statsErr == true {
					req.Response = errors.New("error reading stats from Openperf.")
					statsErr = false
				} else {
					req.Response = makeOPStatsResponse()
				}
				close(req.Done)
			}
		case <-handler.ErrorRunstate:
			runstateErr = true
		case <-handler.ErrorStats:
			statsErr = true
		}
	}
}
