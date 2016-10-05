import os
import distutils
from messages import *

BOARDS_PATH = 'src/platform/boards/'
DEFAULT_CONFIG = 'platform_default.conf'
BOARD_CONFIG = 'platform.conf'
SIMULATION_CONFIG = 'src/omnetpp/simulation.conf'

class Conf():
	def load(self,arguments,conf_file,cometos_path):
		self.conf_file = conf_file
		self.conf_path = os.path.realpath(conf_file)
		self.cometos_path = cometos_path

		# Initialize configuration dictionaries
		# self.new_conf is filled by the configuration scripts
		# self.conf will hold the final configuration value and the source of this configuration
		self.conf = {}
		self.new_conf = {'default_conf':self.default_conf} # make include available as function in configs

		# Get only platform configuration (loading the configuration depends on this)
		#self.new_conf['platform'] = ''
		#if arguments.has_key('platform'):
		#	self.new_conf['platform'] = arguments['platform']
		#	self.update_config('command line argument')
		#elif not self.conf_file == '':
		#	tmpconf = self.new_conf # backup new_conf - we only want to have platform in there
		#	self.execute_conffile(self.conf_path)
		#	if self.new_conf.has_key('platform'):
		#		tmpconf['platform'] = self.new_conf['platform']
		#		self.new_conf = tmpconf
		#		self.update_config(self.conf_file)

		# Existing values are not overwritten by later configurations

		# Read command line parameters
		self.current_source = 'command line argument'
		self.new_conf.update(arguments)
		self.update_config()

		# Read project specific configuration
		if not self.conf_file == '':
			self.execute_conffile(self.conf_path)

		if not self.conf.has_key('platform') or self.conf['platform'] == '':
			error_platform_unspecified()
			exit(1)

		# TODO put omnet config into board/omnet/
		if self.str('platform') == 'omnet':
			# Read simulation configuration
			path = SIMULATION_CONFIG
			self.execute_conffile(os.path.join(self.cometos_path,path))
		else:
			# Read platform specific configuration
			path = os.path.join(BOARDS_PATH,self.conf['platform']['value'],BOARD_CONFIG)
			self.execute_conffile(os.path.join(self.cometos_path,path))

                # Read default platform configuration
                path = os.path.join(BOARDS_PATH,DEFAULT_CONFIG)
                self.execute_conffile(os.path.join(self.cometos_path,path))

		# Print the configuration
		print_config(self.conf)

	def execute_conffile(self,path):
		self.current_source = path
		self.next_default_conf = ''
		current_dir = os.getcwd()
		p = os.path.split(path)
		os.chdir(p[0])
		execfile(p[1],self.new_conf)
		os.chdir(current_dir)
		self.update_config()

		if self.next_default_conf:
			self.execute_conffile(self.next_default_conf)

	# the given conf will be executed after executing the current conf
	def default_conf(self,path):
		self.next_default_conf = os.path.realpath(path)

	def str(self,key,valid_values=[]):
		if not self.conf.has_key(key):
			error_config_unspecified(key,valid_values)
			exit(1)

		value = self.conf[key]['value']

		if valid_values != [] and not str(value) in map(lambda x: str(x), valid_values):
			error_invalid_config(key,value,valid_values)
			exit(1)

		return str(value)


	def bool(self,key):
		valid_values = [True,False]

		if not self.conf.has_key(key):
			error_config_unspecified(key,valid_values)
			exit(1)

		if not isinstance(self.conf[key]['value'],bool):
			try:
				self.conf[key]['value'] = distutils.util.strtobool(str(self.conf[key]['value']))
			except ValueError:
				error_invalid_config(key,self.conf[key]['value'],['True','False'])
				exit(1)

		return self.conf[key]['value']

	def list(self, key):
		if not self.conf.has_key(key):
			error_config_unspecified(key)
			exit(1)
		
		value = self.conf[key]['value']

                if isinstance(value,basestring):
                    value = value.replace(" ","")
                    valueList = value.split(",")
		    return valueList
                else:
                    return value
		

	def update_config(self):
		source = self.current_source
		if source != 'command line argument':
			source = os.path.realpath(source)
			source = source.replace(os.path.realpath(os.getcwd())+'/','')
			source = source.replace(os.path.realpath(self.cometos_path),'$COMETOS_PATH')

		for k, v in self.new_conf.items():
			# Existing values are not overwritten by later configurations
			if not self.conf.has_key(k):
				self.conf[k] = {'value':v,'source':source} 
			


	def get_keys(self):
		return self.conf.keys()


	def has_key(self,key):
		return self.conf.has_key(key)
