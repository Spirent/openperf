# -*- coding: utf-8 -*-

import runnable
from mamba.infrastructure import code_coverage
from mamba.runners import BaseRunner as MambaBaseRunner


class BaseRunner(MambaBaseRunner):

    def _run_examples_in(self, module):
        for example in self.loader.load_examples_from(module):
            example.execute(self.reporter,
                            runnable.ExecutionContext(),
                            tags=self.tags)

            if example.failed():
                self._has_failed_examples = True
