# -*- coding: utf-8 -*-

from mamba.runnable import Runnable as MambaRunnable


class ExecutionSkippedException(Exception):
    pass


class ExecutionContext(object):
    def skip(self):
        raise ExecutionSkippedException


class Runnable(MambaRunnable):

    def __init__(self, parent=None, tags=None):
        super().__init__(parent, tags)
        self._skipped = False

    def failed(self):
        return not self._skipped and self.error is not None

    def skipped(self):
        return self._skipped

    def skip(self):
        self._skipped = True
