from SCons.Script import *

class ATmegaToolchain:
    def __init__(self,build,env):
        self.build = build
        self.env = env
    
    def execute(self,name,source_dirs,build_dir):
        # Defines    
        self.env.Append(CPPDEFINES=['REDUCED_PARAM_CHECK', 'BOARD_'+self.env.conf.str('platform'),'F_CPU=16000000UL'])

        # Compiler
        self.env.Append(CXXFLAGS='-fno-exceptions -fno-rtti -fno-threadsafe-statics')
        self.env.Append(CCFLAGS='-mmcu='+self.env.conf.str('mcu')+' -gdwarf-2 -Wl,-Map=Device.map -ffunction-sections -fdata-sections -Wall -Wundef -Os')
        self.env['LINKFLAGS'] = '-Wl,--section-start=.stackprot='+self.env.conf.str('atmega_stackprot')+' -Wl,--defsym=__heap_end='+self.env.conf.str('atmega_heap_end')+' -mmcu='+self.env.conf.str('mcu')+' -gdwarf-2   -Wl,-Map='+name+'.map -Wl,--section-start=.data=0x800200 -mrelax -Wl,--gc-sections'

        if self.env.bootloader:
            self.env['LINKFLAGS'] += ' -Wl,--section-start=.text=0x3E000'

        self.env.Append(LIBS=['c','m','gcc'])

        # Hex Builder
        elf_to_hex = Builder(action = 'avr-size $SOURCE && avr-objcopy -O ihex -R .eeprom -R .stackprot $SOURCE $TARGET')
        elf_to_lss = Builder(action = 'avr-objdump -h -d -C $SOURCE > $TARGET')
        
        self.env.Append(BUILDERS = {'ElfToHex': elf_to_hex})
        self.env.Append(BUILDERS = {'ElfToLss': elf_to_lss})
        
        # Merge with bootloader
        if self.env.bootloader_program:
            merge_with_bootloader = Builder(action = 'head -n-1 ${SOURCES[0]} | cat - ${SOURCES[1]} > $TARGET')
            self.env.Append(BUILDERS = {'MergeWithBootloader': merge_with_bootloader})

        def build_image(env, target, source):
            elfname = os.path.splitext(target)[0]+'.elf'
            lssname = os.path.splitext(target)[0]+'.lss'
            elf = env.Program(elfname,source)
            
            # Convert to hex
            img = env.ElfToHex(target,elf)
            self.env.ElfToLss(lssname, elfname)

            # Merge with bootloader
            if self.env.bootloader_program:
                blname = os.path.splitext(target)[0]+'_bl.hex'
                return env.MergeWithBootloader(blname,[img,self.env.bootloader_program])
            else:
                return img

        self.env.AddMethod(build_image, "Image")

        # Build the image
        hardware = self.env.Image(name+'.hex', self.env.get_objs())
        self.env.Alias('build', hardware)

        # Define flash target
        if not hardware:
            print("No objects for hardware defined. Correct SConscript files available in your source_dir?")
            exit(1)

        # Handle programmer
        programmer = self.env.conf.str('programmer')
        programmer_port = self.env.conf.str('programmer_port')
        if programmer == 'olimex':
            programmer = 'stk500v2'
            if programmer_port == 'default':
                programmer_port = '/dev/olimex-isp'
        else:
            if programmer_port == 'default':
                programmer_port = 'usb'

        # Build up flash command
        if not self.env.bootloader: # bootloader will be flashed together with the program image
            ## if bootloader flag is given, we need to change the fuses
            hfuseVal = self.env.conf.str('hfuse')
            lfuseVal = self.env.conf.str('lfuse')
            if self.env.conf.bool('bootloader'):
                ## program LSB of high fuse byte to change reset vector to bootloader
                hfuseVal = str(int(hfuseVal,16) & ~0x01)
                

            if self.env.conf.has_key('programmer_id'):
                programmer_port += ':'+self.env.conf.str('programmer_id')
            flash_cmd = "avrdude "+self.env.conf.str('programmer_flags')+' -Ulfuse:w:'+lfuseVal+':m -Uhfuse:w:'+hfuseVal+':m' \
                 +' -p'+self.env.conf.str('programmer_mcu')+' -c'+programmer+' -P'+programmer_port+' -Uflash:w:'+str(hardware[0])+':a'

            AlwaysBuild(self.env.Alias('go', [hardware], flash_cmd))
            AlwaysBuild(self.env.Alias('gocmd', [hardware], "echo "+flash_cmd+" > program.sh"))
            AlwaysBuild(self.env.Alias('dump', [hardware], 'avr-objdump -h -S '+name+'.elf > '+name+'.lst'))

        return hardware

