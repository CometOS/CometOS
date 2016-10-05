class Entry():
    def __init__(self, node, string, nvSep, srcSepIn):
        self.node = node
        self.nvSep = nvSep
        self.srcSepIn = srcSepIn 
        s = self.parse(string)
        if node == None:
            self.nameString = s
        else:
            self.nameString = "node[" + str(self.node) + "]." + s
                
    def parse(self, string):
        tmp = string.split(self.nvSep)
        assert len(tmp) == 2
        self.value = tmp[1]
        nameString = tmp[0]
        tmp2 = nameString.split(self.srcSepIn)
        if len(tmp2) == 2:
            self.src = tmp2[1]
        else:
            self.src = None
            assert len(tmp2) == 1
        self.name = tmp2[0]
        return nameString

