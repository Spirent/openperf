import durationpy
import validators
import yaml


class Config(object):
    def __init__(self, path):
        with open(path, "r") as f:
            data = yaml.load(f)
        self.services = dict([(k, ServiceConfig(k, **v)) for k, v in data['services'].items()])

    def service(self, name='default'):
        return self.services[name]


class ServiceConfig(object):
    def __init__(self, name, **kwargs):
        if not name:
            raise Exception('missing name')

        self.name = name

        def set_str(k, v):
            self.__dict__[k] = str(v)

        def set_url(k, v):
            v = str(v)
            if not validators.url(v):
                raise ValueError('%s service config has invalid url: %s' % (name, v))
            self.__dict__[k] = v

        def set_duration(k, v):
            v = str(v)
            try:
                # d is a datetime.timedelta
                d = durationpy.from_str(v)
            except Exception:
                raise ValueError('%s service config has invalid duration: %s' % (name, v))
            self.__dict__[k] = d

        setters = {
            'command': set_str,
            'base_url': set_url,
            'init_url': set_url,
            'init_timeout': set_duration,
        }

        for k, v in kwargs.items():
            setter = setters.get(k)
            if not setter:
                raise KeyError('%s service config has unknown property: %s' % (name, k))
            setter(k, v)

        for k in ('command', 'base_url'):
            if k not in self.__dict__:
                raise Exception('%s service config is missing required property: %s' % (name, k))
