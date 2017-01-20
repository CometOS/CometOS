# Generated by YCM Generator at 2017-01-17 14:04:59.000751

# This file is NOT licensed under the GPLv3, which is the license for the rest
# of YouCompleteMe.
#
# Here's the license text for this file:
#
# This is free and unencumbered software released into the public domain.
#
# Anyone is free to copy, modify, publish, use, compile, sell, or
# distribute this software, either in source code form or as a compiled
# binary, for any purpose, commercial or non-commercial, and by any
# means.
#
# In jurisdictions that recognize copyright laws, the author or authors
# of this software dedicate any and all copyright interest in the
# software to the public domain. We make this dedication for the benefit
# of the public at large and to the detriment of our heirs and
# successors. We intend this dedication to be an overt act of
# relinquishment in perpetuity of all present and future rights to this
# software under copyright law.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.
#
# For more information, please refer to <http://unlicense.org/>

import os
import ycm_core

flags = [
    '-x',
    'c++',
    '-DHAVE_PCAP',
    '-DHAVE_SWAPCONTEXT',
    '-DWITH_MPI',
    '-DWITH_NETBUILDER',
    '-DWITH_PARSIM',
    '-DWITH_QTENV',
    '-DWITH_TKENV',
    '-DXMLPARSER=libxml',
    '-I.',
    '-I../cometos_externals',
    '-I../cometos_externals/MiXiM/base/connectionManager',
    '-I../cometos_externals/MiXiM/base/messages',
    '-I../cometos_externals/MiXiM/base/modules',
    '-I../cometos_externals/MiXiM/base/phyLayer',
    '-I../cometos_externals/MiXiM/base/utils',
    '-I../cometos_externals/MiXiM/modules/analogueModel',
    '-I../cometos_externals/MiXiM/modules/messages',
    '-I../cometos_externals/MiXiM/modules/phy',
    '-I../cometos_externals/MiXiM/modules/utility',
    '-I/home/cto7138/omnetpp/omnetpp-5.0/include',
    '-Iexamples',
    '-Iexamples/common',
    '-Iexamples/common/aggregation',
    '-Iexamples/common/comm',
    '-Iexamples/common/mac',
    '-Iexamples/common/mp',
    '-Iexamples/common/sniffer',
    '-Iexamples/common/timesync',
    '-Iexamples/common/traffic',
    '-Iexamples/simulation',
    '-Iexamples/simulation/aggregation',
    '-Iexamples/simulation/base',
    '-Iexamples/simulation/comm',
    '-Iexamples/simulation/config',
    '-Iexamples/simulation/distributed',
    '-Iexamples/simulation/ipv6',
    '-Iexamples/simulation/ipv6/6lowpan',
    '-Iexamples/simulation/ipv6/RPL',
    '-Iexamples/simulation/ipv6/cfg',
    '-Iexamples/simulation/ipv6/cfg/1-10-Network',
    '-Iexamples/simulation/ipv6/cfg/1-3-Network',
    '-Iexamples/simulation/ipv6/cfg/3-8-Network',
    '-Iexamples/simulation/ipv6/cfg/8-2-Network',
    '-Iexamples/simulation/ipv6/cfg/Grid-10x10-Network',
    '-Iexamples/simulation/ipv6/cfg/Grid-4x4-Network',
    '-Iexamples/simulation/ipv6/cfg/Grid-6x6-Network',
    '-Iexamples/simulation/ipv6/cfg/Grid-8x8-Network',
    '-Iexamples/simulation/ipv6/cfg/LFFRSimNetwork',
    '-Iexamples/simulation/ipv6/cfg/Random-100-Network-1396346073.83',
    '-Iexamples/simulation/ipv6/cfg/Random-16-Network-1396346053.37',
    '-Iexamples/simulation/ipv6/cfg/Random-36-Network-1396346060.26',
    '-Iexamples/simulation/ipv6/cfg/Random-64-Network-1396346066.79',
    '-Iexamples/simulation/ipv6/cfg/RealSim',
    '-Iexamples/simulation/ipv6/cfg/RealSim_2014-11',
    '-Iexamples/simulation/ipv6/cfg/Tree-Network',
    '-Iexamples/simulation/ipv6/cfg/X-Network',
    '-Iexamples/simulation/ipv6/cfg/Y-Network',
    '-Iexamples/simulation/ipv6/cfg/common',
    '-Iexamples/simulation/ipv6/cfg/testNwk1',
    '-Iexamples/simulation/ipv6/cfg/testNwk2',
    '-Iexamples/simulation/ipv6/coap',
    '-Iexamples/simulation/mac',
    '-Iexamples/simulation/mac/base',
    '-Iexamples/simulation/mac/ccarange',
    '-Iexamples/simulation/mac/dsme',
    '-Iexamples/simulation/mac/dsme/results',
    '-Iexamples/simulation/mac/interference',
    '-Iexamples/simulation/mac/multichannel',
    '-Iexamples/simulation/mac/range',
    '-Iexamples/simulation/mac/rangePower',
    '-Iexamples/simulation/mac/timings',
    '-Iexamples/simulation/mp',
    '-Iexamples/simulation/neighborhood',
    '-Iexamples/simulation/node',
    '-Iexamples/simulation/realtime',
    '-Iexamples/simulation/realtime/basic',
    '-Iexamples/simulation/realtime/rtt',
    '-Iexamples/simulation/realtime/test',
    '-Iexamples/simulation/sniffer',
    '-Iexamples/simulation/statistics',
    '-Iexamples/simulation/timesync',
    '-Iexamples/simulation/topologies',
    '-Iexamples/simulation/topologies/custom',
    '-Iexamples/simulation/topologies/grid',
    '-Iexamples/simulation/topologies/line',
    '-Isrc',
    '-Isrc/auxiliary',
    '-Isrc/communication',
    '-Isrc/communication/addressing',
    '-Isrc/communication/addressing/omnetpp',
    '-Isrc/communication/addressing/platform',
    '-Isrc/communication/algorithms',
    '-Isrc/communication/association',
    '-Isrc/communication/base',
    '-Isrc/communication/bounce',
    '-Isrc/communication/bridge',
    '-Isrc/communication/configurator',
    '-Isrc/communication/counter',
    '-Isrc/communication/crc',
    '-Isrc/communication/dsme',
    '-Isrc/communication/dsme/helper',
    '-Isrc/communication/dsme/openDSME',
    '-Isrc/communication/dsme/openDSME/dsmeAdaptionLayer',
    '-Isrc/communication/dsme/openDSME/dsmeLayer',
    '-Isrc/communication/dsme/openDSME/dsmeLayer/ackLayer',
    '-Isrc/communication/dsme/openDSME/dsmeLayer/associationManager',
    '-Isrc/communication/dsme/openDSME/dsmeLayer/beaconManager',
    '-Isrc/communication/dsme/openDSME/dsmeLayer/capLayer',
    '-Isrc/communication/dsme/openDSME/dsmeLayer/gtsManager',
    '-Isrc/communication/dsme/openDSME/dsmeLayer/messageDispatcher',
    '-Isrc/communication/dsme/openDSME/dsmeLayer/messages',
    '-Isrc/communication/dsme/openDSME/dsmeLayer/neighbors',
    '-Isrc/communication/dsme/openDSME/helper',
    '-Isrc/communication/dsme/openDSME/interfaces',
    '-Isrc/communication/dsme/openDSME/mac_services',
    '-Isrc/communication/dsme/openDSME/mac_services/dataStructures',
    '-Isrc/communication/dsme/openDSME/mac_services/mcps_sap',
    '-Isrc/communication/dsme/openDSME/mac_services/mlme_sap',
    '-Isrc/communication/dsme/openDSME/mac_services/mlme_sap/helper',
    '-Isrc/communication/dsme/openDSME/mac_services/pib',
    '-Isrc/communication/http',
    '-Isrc/communication/ieee802154',
    '-Isrc/communication/ieee802154/mac',
    '-Isrc/communication/ieee802154/mac/omnetpp',
    '-Isrc/communication/ieee802154/mac/omnetpp/mixim',
    '-Isrc/communication/ieee802154/phy',
    '-Isrc/communication/injector',
    '-Isrc/communication/ipv6',
    '-Isrc/communication/ipv6/addressing',
    '-Isrc/communication/ipv6/coap',
    '-Isrc/communication/ipv6/coap/messages',
    '-Isrc/communication/ipv6/coap/resources',
    '-Isrc/communication/ipv6/icmp',
    '-Isrc/communication/ipv6/ipFwd',
    '-Isrc/communication/ipv6/ipHeaders',
    '-Isrc/communication/ipv6/ipHeaders/extensionHeaders',
    '-Isrc/communication/ipv6/lowpan',
    '-Isrc/communication/ipv6/lowpan/assembly',
    '-Isrc/communication/ipv6/lowpan/direct',
    '-Isrc/communication/ipv6/lowpan/headerCompression',
    '-Isrc/communication/ipv6/lowpan/specifications',
    '-Isrc/communication/ipv6/messages',
    '-Isrc/communication/ipv6/nd',
    '-Isrc/communication/ipv6/networks',
    '-Isrc/communication/ipv6/nodes',
    '-Isrc/communication/ipv6/routing',
    '-Isrc/communication/ipv6/routing/RPL',
    '-Isrc/communication/ipv6/routing/StaticRouting',
    '-Isrc/communication/ipv6/traffic',
    '-Isrc/communication/ipv6/udp',
    '-Isrc/communication/linkquality',
    '-Isrc/communication/mis',
    '-Isrc/communication/neighborhood',
    '-Isrc/communication/otap',
    '-Isrc/communication/power',
    '-Isrc/communication/print',
    '-Isrc/communication/reliability',
    '-Isrc/communication/routing',
    '-Isrc/communication/serialize',
    '-Isrc/communication/systemmonitor',
    '-Isrc/communication/tcp',
    '-Isrc/communication/time',
    '-Isrc/communication/topology',
    '-Isrc/communication/traffic',
    '-Isrc/communication/transformation',
    '-Isrc/core',
    '-Isrc/core/omnetpp',
    '-Isrc/omnetpp',
    '-Isrc/pal',
    '-Isrc/templates',
    '-Wall',
    '-std=c++11',
]


# Set this to the absolute path to the folder (NOT the file!) containing the
# compile_commands.json file to use that instead of 'flags'. See here for
# more details: http://clang.llvm.org/docs/JSONCompilationDatabase.html
#
# You can get CMake to generate this file for you by adding:
#   set( CMAKE_EXPORT_COMPILE_COMMANDS 1 )
# to your CMakeLists.txt file.
#
# Most projects will NOT need to set this to anything; you can just change the
# 'flags' list of compilation flags. Notice that YCM itself uses that approach.
compilation_database_folder = ''

if os.path.exists( compilation_database_folder ):
  database = ycm_core.CompilationDatabase( compilation_database_folder )
else:
  database = None

SOURCE_EXTENSIONS = [ '.C', '.cpp', '.cxx', '.cc', '.c', '.m', '.mm' ]

def DirectoryOfThisScript():
  return os.path.dirname( os.path.abspath( __file__ ) )


def MakeRelativePathsInFlagsAbsolute( flags, working_directory ):
  if not working_directory:
    return list( flags )
  new_flags = []
  make_next_absolute = False
  path_flags = [ '-isystem', '-I', '-iquote', '--sysroot=' ]
  for flag in flags:
    new_flag = flag

    if make_next_absolute:
      make_next_absolute = False
      if not flag.startswith( '/' ):
        new_flag = os.path.join( working_directory, flag )

    for path_flag in path_flags:
      if flag == path_flag:
        make_next_absolute = True
        break

      if flag.startswith( path_flag ):
        path = flag[ len( path_flag ): ]
        new_flag = path_flag + os.path.join( working_directory, path )
        break

    if new_flag:
      new_flags.append( new_flag )
  return new_flags


def IsHeaderFile( filename ):
  extension = os.path.splitext( filename )[ 1 ]
  return extension in [ '.H', '.h', '.hxx', '.hpp', '.hh' ]


def GetCompilationInfoForFile( filename ):
  # The compilation_commands.json file generated by CMake does not have entries
  # for header files. So we do our best by asking the db for flags for a
  # corresponding source file, if any. If one exists, the flags for that file
  # should be good enough.
  if IsHeaderFile( filename ):
    basename = os.path.splitext( filename )[ 0 ]
    for extension in SOURCE_EXTENSIONS:
      replacement_file = basename + extension
      if os.path.exists( replacement_file ):
        compilation_info = database.GetCompilationInfoForFile(
          replacement_file )
        if compilation_info.compiler_flags_:
          return compilation_info
    return None
  return database.GetCompilationInfoForFile( filename )


def FlagsForFile( filename, **kwargs ):
  if database:
    # Bear in mind that compilation_info.compiler_flags_ does NOT return a
    # python list, but a "list-like" StringVec object
    compilation_info = GetCompilationInfoForFile( filename )
    if not compilation_info:
      return None

    final_flags = MakeRelativePathsInFlagsAbsolute(
      compilation_info.compiler_flags_,
      compilation_info.compiler_working_dir_ )

  else:
    relative_to = DirectoryOfThisScript()
    final_flags = MakeRelativePathsInFlagsAbsolute( flags, relative_to )

  return {
    'flags': final_flags,
    'do_cache': True
  }
