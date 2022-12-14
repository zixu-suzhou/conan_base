from conans import ConanFile, tools
import os


class PackageConan(ConanFile):
    name = os.environ.get('PACKAGE_NAME')
    version = os.environ.get('PACKAGE_VERSION')
    settings = 'os', 'compiler', 'build_type', 'arch'
    description = 'sample module, basic types, all-in-one serialization'
    url = 'https://devops.momenta.works/Momenta/Mpilot/_git/sample'
    license = 'Momenta Limited, Inc'
    topics = ('sample', 'core', 'basic-types')
    generators = 'cmake'
    requires = ()
    #def imports(self):
    #    self.copy('*.so*',  dst='deploy/common/lib',    src='lib')


    def package(self):
        source_dir = os.environ.get('PACKAGE_SOURCE_DIR')
        build_dir = os.environ.get('PACKAGE_BUILD_DIR')
        install_dir = os.environ.get('PACKAGE_INSTALL_DIR')
        assert self.copy('*', dst='include', src='{}/include'.format(install_dir)), 'not any header files'
        self.copy('*.so', dst='lib', src='{}/lib'.format(install_dir))
        assert self.copy('*', dst='bin', src='{}/bin'.format(install_dir)), 'not any bin files'

    def package_info(self):
        self.cpp_info.libs = tools.collect_libs(self)
