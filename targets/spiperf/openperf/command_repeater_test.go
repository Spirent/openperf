package openperf_test

import (
	"context"
	"time"

	. "github.com/onsi/ginkgo"
	. "github.com/onsi/gomega"

	. "github.com/Spirent/openperf/targets/spiperf/openperf"
)

var _ = Describe("CommandRepeater, ", func() {

	const assertEpsilon = 15 * time.Second

	var (
		opCmdOut chan *Command
		cancelFn context.CancelFunc
		ctx      context.Context
		crReturn chan error
		cr       *CommandRepeater
		command  *Command
	)

	BeforeEach(func() {

		crReturn = make(chan error)
		opCmdOut = make(chan *Command)
		command = &Command{
			Request: &GetTimeRequest{},
		}
		cr = &CommandRepeater{
			Command:        command,
			OpenperfCmdOut: opCmdOut,
		}

	})

	Context("runs, ", func() {

		BeforeEach(func() {
			ctx, cancelFn = context.WithCancel(context.Background())
		})

		Context("using the minimum interval, ", func() {
			BeforeEach(func() {
				cr.Interval = CommandRepeaterMinimumInterval
				cr.Responses = make(chan interface{})
				go func(ctx context.Context, cr *CommandRepeater) {
					crReturn <- RunCommandRepeater(ctx, cr)
				}(ctx, cr)
			})

			Context("produces output multiple times, ", func() {
				It("exits cleanly when canceled", func(done Done) {
					responsesToRead := 3

					opCtx, opCancel := context.WithCancel(context.Background())
					go func(ctx context.Context, reqChan chan *Command) {
					Done:
						for {
							select {
							case <-ctx.Done():
								break Done
							case req := <-reqChan:
								req.Response = &GetTimeResponse{}
								close(req.Done)
							}
						}
					}(opCtx, opCmdOut)

					for i := 0; i < responsesToRead; i++ {
						resp := <-cr.Responses
						Expect(resp).To(BeAssignableToTypeOf(&GetTimeResponse{}))
					}

					cancelFn()

					opCancel()

					Eventually(cr.Responses).Should(BeClosed())

					Eventually(crReturn).Should(Receive(BeNil()))

					close(done)
				}, assertEpsilon.Seconds())

			})
		})

		Context("using an interval of 500ms, ", func() {
			BeforeEach(func() {
				cr.Interval = time.Millisecond * 500
				cr.Responses = make(chan interface{})

				go func(ctx context.Context, cr *CommandRepeater) {
					crReturn <- RunCommandRepeater(ctx, cr)
				}(ctx, cr)
			})

			Context("when the openperf controller doesn't accept the command, ", func() {
				It("terminates when canceled ", func(done Done) {
					Eventually(crReturn).ShouldNot(Receive())

					cancelFn()

					close(done)
				}, assertEpsilon.Seconds())
			})

			Context("when the openperf controller doesn't provide a response, ", func() {
				It("terminates when canceled", func(done Done) {
					<-opCmdOut

					Eventually(crReturn).ShouldNot(Receive())

					cancelFn()

					close(done)
				}, assertEpsilon.Seconds())
			})

			Context("produces output multiple times, ", func() {
				It("exits cleanly when canceled", func(done Done) {
					responsesToRead := 3

					opCtx, opCancel := context.WithCancel(context.Background())
					go func(ctx context.Context, reqChan chan *Command) {
					Done:
						for {
							select {
							case <-ctx.Done():
								break Done
							case req := <-reqChan:
								req.Response = &GetTimeResponse{}
								close(req.Done)
							}
						}
					}(opCtx, opCmdOut)

					for i := 0; i < responsesToRead; i++ {
						resp := <-cr.Responses
						Expect(resp).To(BeAssignableToTypeOf(&GetTimeResponse{}))
					}

					cancelFn()

					opCancel()

					Eventually(cr.Responses).Should(BeClosed())

					Eventually(crReturn).Should(Receive(BeNil()))

					close(done)
				}, assertEpsilon.Seconds())

			})
		})
	})
})
