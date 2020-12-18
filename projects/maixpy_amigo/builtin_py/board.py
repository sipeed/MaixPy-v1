
class board_info:
  def set(key, value=None):
    return setattr(__class__, key, value)
  def all():
    return dir(__class__)
  def get():
    return getattr(__class__, key)
  def load(__map__={}):
    for k, v in __map__.items():
      __class__.set(k, v)

from Maix import config
tmp = config.get_value('board_info', None)
if tmp != None:
    board_info.load(tmp)
else:
    print('[Warning] Not loaded from /flash/config.json to board_info.')
