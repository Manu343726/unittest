from conans import python_requires

common = python_requires('conan_common_recipes/0.0.8@Manu343726/testing')

class UnittestConan(common.CMakePackage):
    name = 'unittest'
    version = '0.0.0'
    license = 'MIT'
    requires = ('tinyrefl/0.4.0@Manu343726/testing',
                'ctti/0.0.2@Manu343726/testing',
                'fmt/5.3.0@bincrafters/stable',
                'CTRE/v2.4@ctre/stable',
                'backward/1.3.1@Manu343726/stable',
                'elfspy/master@Manu343726/testing')
    build_requires = 'tinyrefl-tool/0.4.0@Manu343726/testing'
    generators = 'cmake'
