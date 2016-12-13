from SCons.Script import *
import subprocess
import sys
import select
import socket

PORT = 2331

class CortexToolchain:
	def __init__(self,build,env):
		self.build = build
		self.env = env
	
	def execute(self,name,source_dirs,build_dir):
		# Defines	
		self.env.Append(CPPDEFINES=['USE_FULL_ASSERT'])	
		self.env.Append(CPPDEFINES=['BOARD_'+self.env.conf.str('platform')])

		# Compiler
		flags='-mcpu='+self.env.conf.str('cpu')+' -mthumb -fmessage-length=0 -fdata-sections -ffunction-sections -g3 -funwind-tables'

                if self.env.conf.bool('optimize_size'):
                    flags += ' -Os'
                else:
                    flags += ' -O0'

		flags+= " -mfloat-abi="
		if self.env.conf.bool('hardfloat'):
			flags+="hard -mfpu="+self.env.conf.str('mfpu')
		else:
			flags+="soft"    
            
		self.env.Append(CXXFLAGS=' -fabi-version=0 -fno-exceptions -fno-rtti -fno-use-cxa-atexit -fno-threadsafe-statics')
		self.env.Append(CCFLAGS=flags)
		self.env.Append(ASFLAGS=self.env['CCFLAGS']) # use the previous and all settings defined elsewhere to compile .S files
		self.env.Append(ASFLAGS='-x assembler-with-cpp')
		self.env.Append(LINKFLAGS=flags)
		
		ldflags = ""
		for linkerScript in self.env.conf.list('ld_scripts'):
			ldflags += '-T "' + linkerScript + '" '
		ldflags+="-L" +self.build.cometos_path+"/"+self.env.conf.str("ld_script_path") + " "
		ldflags+=self.env.conf.str('ld_additional_args') + " "
		print ldflags
		self.env.Append(LINKFLAGS=ldflags + ' -Xlinker --gc-sections --specs=nano.specs')

		# Builder
		def build_image(env, target, source):
			return env.Program(target,source)

		self.env.AddMethod(build_image, "Image")

		# Build the image
		hardware = self.env.Image(name+'.elf', self.env.get_objs())
		self.env.Alias('build', hardware)

		# Define flash target
		if not hardware:
			print("No objects for hardware defined. Correct SConscript files available in your source_dir?")
			exit(1)

		# Build up flash command
		def flash_image(env, target, source):
                        use_openocd = (self.env.conf.str("cpu") == "cortex-m3")

			print source
			p = None
                        #if subprocess.call(['ps','-C','JLinkGDBServer','--no-headers']) == 0:
                        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                        result = sock.connect_ex(('127.0.0.1',PORT))
                        if result == 0:
                            print "GDB server is already running. Trying to use that instance!"
                        else:
                            if use_openocd:
                                print "OpenOCD is not running. Starting it temporarily!"

                                with open("openocd.cfg",'w') as f:
                                    f.write('\n'.join([
                                        'interface ftdi',
                                        'ftdi_vid_pid 0x0403 0x6010',

                                        'ftdi_layout_init 0x0c08 0x0c2b',
                                        'ftdi_layout_signal nTRST -data 0x0800',
                                        'ftdi_layout_signal nSRST -data 0x0400',

                                        'source [find target/stm32f1x.cfg]',

                                        'adapter_khz 1500',
                                        'gdb_port '+str(PORT)
                                        ]))

                                cmd = ['openocd','-f','openocd.cfg']
                                polltime = 1000
                            else:
                                print "JLinkGDBServer is not running. Starting it temporarily!"
                                cmd = ['JLinkGDBServer','-device','MK64FN1M0xxx12','-if','SWD']
                                polltime = 500

                            p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
                            poller = select.poll()
                            poller.register(p.stdout)

                            # wait for gdb server to be ready 
                            while True:
                                evts = poller.poll(polltime)
                                if evts == []:
                                        break 
                                for evt in evts:
                                    if evt[1] == select.POLLHUP:
                                        print ""
                                        print "GDB server closed."
                                        print "Check the connection to your device!"
                                        exit(1)
                                    else:
                                        sys.stdout.write(p.stdout.read(1))
                                        sys.stdout.flush()

			self.gdb = subprocess.Popen('arm-none-eabi-gdb 2>&1', stdin=subprocess.PIPE, stdout=subprocess.PIPE, shell=True)

			sys.stdout.write(self.wait_for_prompt('(gdb)'))

			cmds = ['set breakpoint pending on',
			'enable pretty printing',
			'set python print-stack none',
			'set print object on',
			'set print sevenbit-strings on',
			'set charset ISO-8859-1',
			'set mem inaccessible-by-default off',
			'set auto-solib-add on']

			self.execute_cmds(cmds)

			result = self.execute_cmds(['target remote localhost:2331'])
			if 'Connection timed out' in result:
				self.quit_gdb()
				print "Connection to GDB server timed out"
				exit(1)

			cmds = [
			'monitor speed 1000',
			'monitor clrbp',
			'monitor reset',
			'monitor halt',
			'monitor regs',
			'flushreg',
			'monitor speed auto',
			'monitor flash breakpoints 1',
			'monitor semihosting enable',
			'monitor semihosting IOClient 1',
			'symbol-file '+str(source[0]),
			'load '+str(source[0]),
			'monitor clrbp',
			'monitor reset']

			self.execute_cmds(cmds)

                        if not use_openocd:
                            cmds = [
                            'monitor regs',
                            'monitor halt',
                            'flushreg',
                            'monitor go']
                            self.execute_cmds(cmds)


			self.quit_gdb()

			if p != None:
				p.terminate()
				time.sleep(1)
                

		bld = Builder(action = flash_image, suffix = '', src_suffix = '.elf')
		env = Environment(BUILDERS = {'Flash' : bld})
		if "go" in BUILD_TARGETS:
			AlwaysBuild(env.Alias('go',env.Flash(hardware)))

	def wait_for_prompt(self,prompt):
		output = ''
		while True:
			char = self.gdb.stdout.read(1)
			output += char
			if output.endswith(prompt):
				break;
		return output

	def execute_cmds(self,cmds):
		total_output = ''
		for cmd in cmds:
			print ' '+cmd
			self.gdb.stdin.write(cmd+'\n')
			self.gdb.stdin.flush()
			output = self.wait_for_prompt('(gdb)').lstrip()
			total_output += output
			sys.stdout.write(output)
		return total_output

	def quit_gdb(self):
		self.gdb.stdin.write('quit\n')
		print ' quit'
		self.gdb.wait()
