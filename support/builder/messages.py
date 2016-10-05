import traceback

#######################################################
#          Definition of console messages             #
#######################################################

def print_config(config):
	print("""
-----------------------------------------------------------------------------
Active Configuration:
		""")
	config_keys = list(set(config.keys()) - set(globals().keys()) - set(['default_conf','platform']))
	config_keys.sort()

	if 'platform' in config.keys():
		config_keys.insert(0,'platform')

	row_format = "{:<26} = {:<20}   ({:})"
	for k in config_keys:
		v = config[k]['value']
		if isinstance(v,bool):
			v = 'True' if v else 'False'
		print row_format.format(k,v,config[k]['source'])

	print("""
-----------------------------------------------------------------------------\n""")


def error_platform_unspecified():
	print("""
-----------------------------------------------------------------------------
ERROR: Platform is not specified

You have two options:
1.) Provide it to the cob call:
    cob platform=something
				  
2.) Define it in your project specific configuration file:
    platform='something'
-----------------------------------------------------------------------------\n""")


def error_config_unspecified(key,valid_values):
	print('In file '+traceback.extract_stack()[-3][0])
	print('At line '+str(traceback.extract_stack()[-3][1]))
	print('In command: '+traceback.extract_stack()[-3][3])
	print("""
-----------------------------------------------------------------------------
ERROR: Configuration parameter '"""+key+"""' is not specified!\n""")
	if(valid_values != []):
            print("Valid options are: """+', '.join(map(lambda x: str(x), valid_values)))

	print("""
You have several options:
1.) Provide it to the cob call:
    cob """+key+"""=something
				  
2.) Define it in your project specific configuration file:
    """+key+"""='something'
				
3.) Define it in the board specific configuration file of CometOS,
    but only if you can make sure that it will not break anything for anyone! 
-----------------------------------------------------------------------------\n""")


def error_invalid_config(key,value,valid_values):
	print('In file '+traceback.extract_stack()[-3][0])
	print('At line '+str(traceback.extract_stack()[-3][1]))
	print('In command: '+traceback.extract_stack()[-3][3])
	print("""
--------------------------------------------------------------------------------------------
ERROR: Invalid value '"""+str(value)+"""' for configuration parameter '"""+key+"""'!
Valid options are: """+', '.join(map(lambda x: str(x), valid_values))+"""
--------------------------------------------------------------------------------------------\n""")

def error_no_externals():
	print("""
--------------------------------------------------------------------------------------------
ERROR: This build requires external components not shipped with the main CometOS release.

You can find these in the "cometos_externals" repository. If you can ensure the realization
of the license terms of this repository, download the components and provide the
COMETOS_EXTERNALS_PATH environment variable.

--------------------------------------------------------------------------------------------\n""")
