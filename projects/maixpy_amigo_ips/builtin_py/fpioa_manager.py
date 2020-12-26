from Maix import FPIOA

class fm:
  fpioa = FPIOA()

  def help():
    __class__.fpioa.help()

  def get_pin_by_function(function):
    return __class__.fpioa.get_Pin_num(function)

  def register(pin, function, force=True):
    pin_used = __class__.get_pin_by_function(function)
    if pin_used == pin:
      return 
    if None != pin_used:
      info = "[Warning] function is used by %s(pin:%d)" % (
          fm.str_function(function), pin_used)
      if force == False:
        raise Exception(info)
      else:
        print(info)
    __class__.fpioa.set_function(pin, function)

  def unregister(pin):
    __class__.fpioa.set_function(pin, fm.fpioa.RESV0)

  def str_function(function):
    if fm.fpioa.GPIOHS0 <= function and function <= fm.fpioa.GPIO7:
      if fm.fpioa.GPIO0 <= function:
        return 'fm.fpioa.GPIO%d' % (function - fm.fpioa.GPIO0)
      return 'fm.fpioa.GPIOHS%d' % (function - fm.fpioa.GPIOHS0)
    return 'unknown'

  def get_gpio_used():
    return [(__class__.str_function(f), __class__.get_pin_by_function(f)) for f in range(fm.fpioa.GPIOHS0, fm.fpioa.GPIO7 + 1)]
