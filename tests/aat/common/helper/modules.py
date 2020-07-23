from expects import *

import client.api
import client.models

def check_modules_exists(api_client, *modules):
    api = client.api.ModulesApi(api_client)
    op_modules = api.list_modules()
    m5s = list(modules)
    for m in op_modules:
        if m.id in m5s:
            m5s.remove(m.id)
    return len(m5s) == 0