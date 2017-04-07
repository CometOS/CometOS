from SCons.Script import *
from messages import *
import glob

def msgc_emitter(target, source, env):
    # tell scons that for every .msg file, there will be two target files 
    # with _m.h and _m.cc suffix
    new_target = list(target)
    for t in target:
        ccfile = "{0}.cc".format(str(t)[:-2])
        new_target.append(Entry(ccfile))
        env.add_sources([ccfile])
    
    return new_target, source

class OmnetToolchain:
    def __init__(self,build,env):
        self.build = build
        self.env = env

    def execute(self,name,source_dirs,build_dir):
        # Define include directories
        self.env.Append(CPPPATH = [Dir(self.env.omnet_path+'/include')])

        # start msg generator
        # retrieve original includes and CPPDEFINES to create the SWIG wrapper and its python wrapper
        includes = ""
        for i in self.env['CPPPATH']:
            if isinstance(i, basestring):
                includes += i
            else:
                includes += " -I{0}".format(i.srcnode())
        
        # ned tool msg compiler for omnetpp .msg files
        msg_files = self.env.get_sources(self.env.cometos_externals_path+'/MiXiM', suffixes=['.msg'])
        msg_files = [self.env.cometos_externals_path+'/MiXiM/' + f for f in msg_files]
        print msg_files
        msg_builder = Builder(action='nedtool -s _m.cc {0} $SOURCES'.format(includes), 
                              suffix="_m.h", 
                              src_suffix=".msg",
                              emitter=msgc_emitter)
        self.env.Append(BUILDERS = {'msg_builder': msg_builder})
        for f in msg_files:
            self.env.msg_builder(f)


        # Linker
        self.env.Append(LIBPATH = [self.env.omnet_path+'/lib/gcc', self.env.omnet_path+'/lib', self.env.cometos_path])

        libs = ['pthread', 'oppmaind', 'oppenvird', 'opplayoutd', 'oppcmdenvd', 'oppenvird', 'oppsimd', 'dl', 'stdc++']
        for lddir in self.env['LIBPATH']:
            for lib in glob.glob(lddir+"/lib*"):
                if 'opptkenvd' in lib:
                    libs.append('opptkenvd')
        self.env.Append(LIBS = libs)

        self.env.Append(LINKFLAGS = "-u _tkenv_lib -Wl,--no-as-needed -u _cmdenv_lib -Wl,--no-as-needed")

        # Library paths
        libpaths = os.environ['LD_LIBRARY_PATH'].split(':')+self.env['LIBPATH']
        self.env.Append(ENV = {'LD_LIBRARY_PATH': ':'.join(libpaths)})

        # Defines	
        self.env.Append(CPPDEFINES=['HAVE_DLOPEN=0'])

        # Check for MiXiM
        if self.env.conf.bool('mixim') and self.build.cometos_externals_path == '':
                error_no_externals() 
                exit(1)

        # Build the program
        program = self.env.Program('simulation',self.env.get_objs())
        simulation = self.env.Alias('build',program)

        # Define run target
        sim_cmd = 'opp_runall -j1 ./'+str(program[0])+' -n '+self.env.ned_folders()+' -c '+self.env.conf.str('config')+' -r '+self.env.conf.str('run')+' '+self.env.conf.str('ini')+' -u '+self.env.conf.str('env')
		
        AlwaysBuild(self.env.Alias('go', [simulation], sim_cmd))
