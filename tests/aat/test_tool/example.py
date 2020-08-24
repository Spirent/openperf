# -*- coding: utf-8 -*-

from datetime import datetime

import runnable
from mamba.example import Example as MambaExample


class Example(MambaExample, runnable.Runnable):

    def execute(self, reporter, execution_context, tags=None):
        assert self.parent is not None
        if not self.included_in_execution(tags):
            return

        self._start(reporter)

        if not self.skipped():
            if not self.failed():
                self.parent.execute_hook('before_each', execution_context)

            if not self.failed() and not self.skipped():
                self._execute_test(execution_context)

            self.parent.execute_hook('after_each', execution_context)

        self._finish(reporter)

    def _execute_test(self, execution_context):
        try:
            if hasattr(self.test, 'im_func'):
                self.test.im_func(execution_context)
            else:
                self.test(execution_context)
        except runnable.ExecutionSkippedException:
            self.skip()
        except Exception:
            self.fail()
        finally:
            self.was_run = True

    def _finish(self, reporter):
        self.elapsed_time = datetime.utcnow() - self._begin
        if self.skipped():
            reporter.example_skipped(self)
        elif self.failed():
            reporter.example_failed(self)
        else:
            reporter.example_passed(self)
