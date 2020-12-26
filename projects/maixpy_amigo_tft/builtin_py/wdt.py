
from machine import WDT

class protect:
    wdt = None
    def start():
        protect.wdt = WDT(id=0, timeout=6000) # protect.stop()
    def keep():
        if protect.wdt != None:
            protect.wdt.feed()
    def stop():
        protect.wdt.stop()
    def restart():
        protect.wdt = None

protect.start()
