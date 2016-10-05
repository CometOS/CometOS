# This Python module defines some helpers for CometOS SCons build

import os
import fnmatch
import glob
import time
from SCons.Script import *
import re
import distutils.util
import shutil

from messages import *
from conf import *

from toolchains.omnet import *
from toolchains.atmega import *
from toolchains.cortex import *
from toolchains.local import *
from toolchains.python import *

class CometOSBuild:
    def __init__(self, source_dir, cometos_conf = '', cometos_path = os.environ['COMETOS_PATH'],
             build_targets = BUILD_TARGETS, build_dir = 'build'):
        self.cometos_path = cometos_path
        self.source_dir = source_dir
        self.cometos_conf = cometos_conf
        self.build_targets = build_targets
        self.build_targets = BUILD_TARGETS
        self.build_dir = build_dir
        
        self.cometos_externals_path = ''
        if os.environ.has_key('COMETOS_EXTERNALS_PATH'):
            self.cometos_externals_path = os.environ['COMETOS_EXTERNALS_PATH']
         
        self.omnet_path = ''
        if os.environ.has_key('OMNET_PATH'):
            self.omnet_path = os.environ['OMNET_PATH']


    def execute(self):
        # Get an environment
        env = self.environment(conf_file = self.cometos_conf)

        name = 'Device'
        if env.conf.has_key('target'):
            self.build_dir = os.path.join(self.build_dir,env.conf.str('target'))
            name = env.conf.str('target')

        if env.conf.has_key('platform'):
            self.build_dir = os.path.join(self.build_dir,env.conf.str('platform'))
            name = '{0}/{1}'.format(os.path.join("bin", env.conf.str('platform')), name)

        # Bootloader
        if env.conf.bool('bootloader'):
            bootloader_env = self.environment(conf_file = os.path.join(self.cometos_path, 'src/platform/boards', env.conf.str('platform'), 'bootloader/bootloader.conf'))
            bootloader_env.bootloader = True
            env.bootloader_program = self.execute_toolchain(bootloader_env,name+"_bootloader",'',self.build_dir+"_bootloader")

        # Program
        source_dirs = []
        if env.conf.has_key('source_dirs'):
            for d in env.conf.list('source_dirs'):
                s = []
                s.append(d)
                while d.startswith('../'):
                    d = d[len('../'):]
                s.append(d)
                source_dirs.append(s)
        else:
            source_dirs = [[".",""]] # only a single source dir - the root

        self.execute_toolchain(env,name,source_dirs,self.build_dir)

    def execute_toolchain(self,env,name,source_dirs,build_dir):
        toolchain = env.conf.str('toolchain',['atmega','cortex','omnet','local','python'])
        if toolchain == 'atmega':
            self.toolchain = ATmegaToolchain(self,env)
        elif toolchain == 'cortex':
            self.toolchain = CortexToolchain(self,env)
        elif toolchain == 'omnet':
            self.toolchain = OmnetToolchain(self,env)
        elif toolchain == 'python':
            self.toolchain = PyWrapToolchain(self, env)
        elif toolchain == 'local':
            self.toolchain = LocalToolchain(self,env)

        # Run the SConscript for collecting the actual build
        Export({'env':env})
        SConscript(self.cometos_path+"/src/SConscript",variant_dir=os.path.join(build_dir,'cometos_build'))			# CometOS

        if self.cometos_externals_path != '':
            SConscript(self.cometos_externals_path+"/SConscript",variant_dir=os.path.join(build_dir,'externals'))

        if source_dirs != '':
            for source_dir in source_dirs:
                SConscript(source_dir[0]+'/SConscript', variant_dir=os.path.join(build_dir,'project',source_dir[1]))            # Project Code

        return self.toolchain.execute(name,source_dirs,build_dir)

    def environment(self, conf_file=''):
        return CometOSEnvironment(self.cometos_path, self.omnet_path, self.cometos_externals_path, conf_file)

def build_object(env, source, additional_flags):
    assertMode = env.conf.str('asserting', ['short', 'long'])
    if assertMode == 'short':
        additional_flags += " -wrapper FileIdWriter.py --no-integrated-cpp"
    return env.Object(source,CPPFLAGS=additional_flags)

class CometOSEnvironment(Environment):

    def __init__(self, cometos_path, omnet_path, cometos_externals_path, conf_file=''):
        Environment.__init__(self,ENV = os.environ)
        self.cometos_path = cometos_path
        self.cometos_externals_path = cometos_externals_path
        self.omnet_path = omnet_path
        self.objs = []
        self.bootloader = False
        self.bootloader_program = None

        self.AddMethod(build_object, "CometosObject")

        # Necessary for starting a GUI from SCons
        if os.environ.has_key('DISPLAY'):
            self['ENV']['DISPLAY'] = os.environ['DISPLAY']

        # Load the configuration
        self.conf = Conf()
        self.conf.load(ARGUMENTS,conf_file,cometos_path)

        # Define PALs from config
        for k in self.conf.get_keys():
            m = re.match('^pal_(.*)',k)
            if m:
                self.conf_to_bool_define([k])

        # Generic settings
        self['CXX'] = self.conf.str('compiler_prefix')+'g++'
        self['CC'] = self.conf.str('compiler_prefix')+'gcc'
        self['AS'] = self.conf.str('compiler_prefix')+'as'
        #self['CC'] = self.conf.str('compiler_prefix')+'gcc'
        self.Append(CXXFLAGS='-std=c++11')
        self.Append(CFLAGS='-std=c99')
        self.Append(CCFLAGS='-Wall -Wundef -fsigned-char')

        # Output
        if ARGUMENTS.get('VERBOSE') != "1":
            self['SHCXXCOMSTR'] = "Compiling shared object $TARGET"
            self['SHLINKCOMSTR'] = "Linking shared $TARGET"
            self['ARCOMSTR'] = "Archiving $TARGET"
            self['CXXCOMSTR'] = "Compiling $TARGET"
            self['CCCOMSTR'] = "Compiling $TARGET"
            self['LINKCOMSTR'] = "Linking $TARGET"

    def get_platform(self):
        return self.conf.str('platform')


    def target(self):
        return t


    def ned_folders(self):
        subdirs = ['src', 'examples']
        if self.conf.bool('inet'):
            subdirs.append('inet/src')
        if self.conf.bool('mixim'):
            subdirs.append(self.cometos_externals_path+'/MiXiM/base')
            subdirs.append(self.cometos_externals_path+'/MiXiM/modules')
        return ':'.join(map(lambda x: os.path.join(self.cometos_path,x), subdirs))
    

    def get_sources(self,root,ignore_paths=[],suffixes=['.c','.cc']):
        ignore_paths = map(lambda x: os.path.join(root,x), ignore_paths)

        # TODO prevent code duplication
        # Search for include and source directories in CometOS
        dirlist = []
        for dir, dirs, files in os.walk(root):
            # Remove all ignored paths
            for d in dirs:
                rel = os.path.abspath(dir+'/'+d)
                if rel in ignore_paths:
                    dirs.remove(d);

            # Append this directory
            dirlist.append(dir)

        # Search for sources
        sources = []
        for d in dirlist:
            files = []
            for suffix in suffixes:
                files += glob.glob(d+'/*{0}'.format(suffix))
            for f in files:
                rel = os.path.relpath(f,root)
                sources.append(rel)

        return sources
    
    def add_sources(self,sources,additional_flags=''):
        for source in sources:
            self.objs.append(self.CometosObject(source,additional_flags=additional_flags))

    
    def get_objs(self):
        return self.objs
    

    def conf_to_bool_define(self,keys):
        for key in keys:
            if not self.conf.has_key(key.lower()):
                error_config_unspecified(key,[])
                exit(1)
            if self.conf.bool(key.lower()):
                self.Append(CPPDEFINES=[key.upper()])


    def conf_to_str_define(self,keys):
        for key in keys:
            if not self.conf.has_key(key.lower()):
                error_config_unspecified(key.lower(),[])
                exit(1)
            self.Append(CPPDEFINES=[key.upper()+'='+self.conf.str(key.lower())])

    def optional_conf_to_str_define(self, keys):
        for key in keys:
            if not self.conf.has_key(key.lower()):
                # we leave the value unspecified
                pass
            else:
                self.Append(CPPDEFINES=[key.upper()+'='+self.conf.str(key.lower())])
                
