project(
	'aylp_asdk_dm',
	'c',
	default_options: ['c_std=gnu17', 'warning_level=3', 'optimization=3']
	#default_options: ['warning_level=everything', 'optimization=3']
	# this is worth occasionally turning on to check
)

incdir = include_directories(['libasdk', 'libaylp'])

c = meson.get_compiler('c')
asdk_dep = c.find_library(
	'ASDK',
	dirs: join_paths(meson.source_root(), 'libasdk'),
	required: true
)
gsl_dep = dependency('gsl')
json_dep = dependency('json-c')
deps = [asdk_dep, gsl_dep, json_dep]

add_global_arguments(['-DASDK_EXPORTS'], language: 'c')

project_source_files = [
	'aylp_asdk_dm.c',
]

shared_library('aylp_asdk_dm', project_source_files,
	name_prefix: '',
	install: true,
	dependencies: deps,
	include_directories: incdir,
	override_options: 'b_lundef=false'
)

