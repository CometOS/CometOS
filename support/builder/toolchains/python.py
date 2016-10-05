from SCons.Script import *

class PyWrapToolchain:
    def __init__(self,build,env):
        self.build = build
        self.env = env
    
    def execute(self,name,source_dir,build_dir):
        # Defines    
        self.env.Append(CPPDEFINES=['BOARD_python'])

        # Compiler
        self.env.Append(CXXFLAGS='')
        self.env.Append(CCFLAGS='-g3 -Wall -Wundef -fPIC')
        self.env['LINKFLAGS'] = '-shared'
        self.env.Append(LIBS=['m','pthread'])

        # Build the program
        swig_build = 'swig'
        name = '{0}/_cometos.so'.format(swig_build)
        
        if self.env.conf.str('libstdpath') != 'default':
            self.env.Append(LINKFLAGS=['-Wl,-rpath={0}'.format(self.env.conf.str('libstdpath'))])
        
        if self.env.conf.has_key('target'):
            name = self.env.conf.str('target')

        # retrieve original includes and CPPDEFINES to create the SWIG wrapper and its python wrapper
        includes = ""
        for i in self.env['CPPPATH']:
            includes += " -I{0}".format(i.srcnode())
            
        flags = ""
        for f in self.env['CPPDEFINES']:
            flags += " -D{0}".format(f)
            
        # now create SWIG builder to build the wrapper target
        cometosWrap = '{0}/cometos_wrap.cc'.format(swig_build)
        run_swig = Builder(action = 'swig -threads -c++ -python {0} {1} -o $TARGET "$SOURCE"'.format(includes, flags))
        self.env.Append(BUILDERS = {'run_swig': run_swig})
        self.env.run_swig(cometosWrap, 'cometos.i')
        
        # add the created wrapper to general sources       
        self.env.add_sources([cometosWrap])
        
        # need to append python header
        self.env.Append(CPPPATH='/usr/include/python2.7')
        
        # create builder to provide an empty __init__.py file in swig module directory
        create_pyini = Builder(action = 'mkdir -p {0} && touch {0}/$TARGET'.format(swig_build))
        self.env.Append(BUILDERS = {'create_pyini': create_pyini})
        self.env.create_pyini(source="cometos.i", target="__init__.py")
        
        prog = self.env.Program(name, self.env.get_objs())
        self.env.Alias('build', prog)
 
        cmdline = './'+name
        if self.env.conf.has_key('cmdline'):
            cmdline = self.env.conf.str('cmdline')
        AlwaysBuild(self.env.Alias('go', [prog], cmdline))
