from SCons.Script import *

class LocalToolchain:
    def __init__(self,build,env):
        self.build = build
        self.env = env
    
    def execute(self,name,source_dir,build_dir):
        # Defines    
        self.env.Append(CPPDEFINES=['BOARD_local'])

        # Compiler
        self.env.Append(CXXFLAGS='')
        self.env.Append(CCFLAGS='-g3 -Wall -Wundef')
        self.env['LINKFLAGS'] = ''
        self.env.Append(LIBS=['m','pthread'])

        # Build the program
        name = 'a.out'
        if self.env.conf.has_key('target'):
            name = self.env.conf.str('target')

        prog = self.env.Program(name, self.env.get_objs())
        self.env.Alias('build', prog)

        cmdline = './'+name
        if self.env.conf.has_key('cmdline'):
            cmdline = self.env.conf.str('cmdline')
        AlwaysBuild(self.env.Alias('go', [prog], cmdline))
