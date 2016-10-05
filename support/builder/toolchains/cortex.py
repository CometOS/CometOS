from SCons.Script import *
import subprocess
import sys
import select

class CortexToolchain:
	def __init__(self,build,env):
		self.build = build
		self.env = env
	
	def execute(self,name,source_dirs,build_dir):
		# Defines	
		self.env.Append(CPPDEFINES=['USE_FULL_ASSERT'])	
		self.env.Append(CPPDEFINES=['BOARD_'+self.env.conf.str('platform')])

		# Compiler
		flags='-mcpu='+self.env.conf.str('cpu')+' -mthumb -Os -fmessage-length=0 -fdata-sections -ffunction-sections -g3 -funwind-tables'
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
			print source
			p = None
                        if subprocess.call(['ps','-C','JLinkGDBServer','--no-headers']) == 0:
                            print "JLinkGDBServer is already running. Trying to use that instance!"
                        else:
                            print "JLinkGDBServer is not running. Starting it temporarily!"
                            p = subprocess.Popen(['JLinkGDBServer','-device','MK64FN1M0xxx12','-if','SWD'], stdout=subprocess.PIPE)
                            poller = select.poll()
                            poller.register(p.stdout)

                            # wait for gdb server to be ready 
                            while True:
                                evts = poller.poll(500)
                                if evts == []:
                                        break 
                                for evt in evts:
                                    if evt[1] == select.POLLHUP:
                                        print ""
                                        print "JLinkGDBServer closed."
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
			'monitor reset',
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
