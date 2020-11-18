
import os


class OS:
    cache = None

    def update():
        OS.cache = os.listdir('/flash')

    def chdir(path):
        pass

    def getcwd(path):
        pass

    def listdir(path):
        #print(path, OS.cache)
        if OS.cache == None:
            OS.update()
        if path == '/' or '/sd' in path:
            return os.listdir(path)
        if '/flash' == path:  # '/flash'
            directory, result = {}, []
            #tmp = [item for item in OS.cache if '/' not in item]
            for item in OS.cache:
                pos = item.find('/')
                if pos == -1:
                    result.append(item)
                else:
                    # directory['/' + item[0:pos]] = True
                    directory[item[0:pos]] = True
            for item in directory.keys():
                result.append(item)
            # print(directory, result)
            return result
        else:  # '/flash/script' and '/script'
            path = path.replace('/flash', '').replace('/', '') + '/'
            return [item.replace(path, '') for item in OS.cache if path in item]


if __name__ == "__main__":

    print(os.listdir('/flash'))

    print(OS.listdir('/'))

    print(OS.listdir('/flash'))

    print(OS.listdir('/flash/script'))

    #print(OS.listdir('/script'))

    #tmp = open('/flash/main.py')

    #tmp.close()

    #print(OS.listdir('/sd'))

    #print(OS.listdir('/sd/res'))

    print(OS.listdir('/flash/script'))  # no
