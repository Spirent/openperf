# -*- coding: utf-8 -*-

import copy

import runnable
from mamba.example_group import ExampleGroup as MambaExampleGroup


class ExampleGroup(MambaExampleGroup, runnable.Runnable):

    def execute(self, reporter, execution_context, tags=None):
        if not self.included_in_execution(tags):
            return

        self._start(reporter)
        try:
            if not self.skipped():
                self.execute_hook('before_all', execution_context)

            for example in self:
                example_execution_context = copy.copy(execution_context)
                self._bind_helpers_to(example_execution_context)
                if self.skipped():
                    example.skip()
                example.execute(reporter,
                                example_execution_context,
                                tags=tags)

            self.execute_hook('after_all', execution_context)
        except Exception:
            self.fail()

        self._finish(reporter)

    def execute_hook(self, hook, execution_context):
        if hook.endswith('_all') and not self.hooks.get(hook):
            return

        if self.parent is not None and hook.startswith("before") and not hook.endswith("_all"):
            self.parent.execute_hook(hook, execution_context)

        for registered in self.hooks.get(hook, []):
            try:
                if hasattr(registered, 'im_func'):
                    registered.im_func(execution_context)
                elif callable(registered):
                    registered(execution_context)
            except runnable.ExecutionSkippedException:
                self.skip()
            except Exception:
                self.fail()

        if self.parent is not None and hook.startswith("after") and not hook.endswith("_all"):
            self.parent.execute_hook(hook, execution_context)
