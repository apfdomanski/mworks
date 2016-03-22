from contextlib import contextmanager
import multiprocessing
import platform
import os
import subprocess
import sys
import urllib


################################################################################
#
# Shared configuration
#
################################################################################


def join_flags(*flags):
    return ' '.join(flags).strip()


assert os.environ['GCC_VERSION'] == 'com.apple.compilers.llvm.clang.1_0'
cc = '/usr/bin/clang'
cxx = '/usr/bin/clang++'
rsync = '/usr/bin/rsync'
xcodebuild = '/usr/bin/xcodebuild'

python = '/usr/bin/python' + os.environ['MW_PYTHON_VERSION']

num_cores = str(multiprocessing.cpu_count())

common_flags = ' '.join(('-arch ' + arch) for arch in
                        os.environ['ARCHS'].split())
common_flags += (' -isysroot %(SDKROOT)s'
                 ' -mmacosx-version-min=%(MACOSX_DEPLOYMENT_TARGET)s'
                 % os.environ)

compile_flags = ('-g -Os -fexceptions -fvisibility=hidden ' +
                 '-Werror=partial-availability ' +
                 common_flags)

cflags = '-std=%(GCC_C_LANGUAGE_STANDARD)s' % os.environ
cxxflags = ('-std=%(CLANG_CXX_LANGUAGE_STANDARD)s '
            '-stdlib=%(CLANG_CXX_LIBRARY)s'
            % os.environ)

link_flags = common_flags

downloaddir = 'download'
path_to_downloaddir = '../' + downloaddir
patchdir = 'patches'
path_to_patchdir = '../../' + patchdir
builddir = 'source'
stagedir = 'stage'
path_to_stagedir = os.path.abspath(stagedir)


################################################################################
#
# Build helpers
#
################################################################################


all_builders = []


def builder(func):
    all_builders.append(func)
    return func


def announce(msg, *args):
    sys.stderr.write((msg + '\n') % args)


def check_call(args, **kwargs):
    announce('Running command: %s', ' '.join(repr(a) for a in args))
    subprocess.check_call(args, **kwargs)


@contextmanager
def workdir(path):
    old_path = os.getcwd()
    announce('Entering directory %r', path)
    os.chdir(path)
    yield
    announce('Leaving directory %r', path)
    os.chdir(old_path)


def download_file(url, filename):
    filepath = path_to_downloaddir + '/' + filename
    if os.path.isfile(filepath):
        announce('Already downloaded file %r', filename)
    else:
        check_call(['/usr/bin/curl', '-#', '-L', '-f', '-o', filepath, url])


def download_archive(url_path, filename):
    download_file(url_path + filename, filename)


def download_archive_from_sf(path, version, filename):
    url = (('http://downloads.sourceforge.net/project/%s/%s/%s'
            '?use_mirror=autoselect') % (path, version, filename))
    return download_file(url, filename)


def remove_directory(path):
    if os.path.isdir(path):
        check_call(['/bin/rm', '-Rf', path])


def unpack_tarfile(filename, outputdir):
    remove_directory(outputdir)
    check_call(['/usr/bin/tar', 'xf', path_to_downloaddir + '/' + filename])


def unpack_zipfile(filename, outputdir):
    remove_directory(outputdir)
    check_call([
        '/usr/bin/unzip',
        '-q',
        path_to_downloaddir + '/' + filename,
        '-d', outputdir,
        ])


def apply_patch(patchfile, strip=1):
    with open(path_to_patchdir + '/' + patchfile) as fp:
        check_call(
            args = ['/usr/bin/patch', '-p%d' % strip],
            stdin = fp,
            )


def get_updated_env(
    extra_compile_flags = '',
    extra_cflags = '',
    extra_cxxflags = '',
    extra_ldflags = '',
    ):

    env = os.environ.copy()
    env.update({
        'CC': cc,
        'CXX': cxx,
        'CFLAGS': join_flags(compile_flags, extra_compile_flags,
                             cflags, extra_cflags),
        'CXXFLAGS': join_flags(compile_flags, extra_compile_flags,
                               cxxflags, extra_cxxflags),
        'LDFLAGS': join_flags(link_flags, extra_ldflags),
        })
    return env


def run_configure_and_make(
    srcdir,
    extra_args = [],
    command = ['./configure'],
    extra_compile_flags = '',
    extra_cflags = '',
    extra_cxxflags = '',
    extra_ldflags = '',
    patchfile = None,
    post_patch_command = None,
    ):

    with workdir(srcdir):
        if patchfile:
            apply_patch(patchfile)
            if post_patch_command:
                check_call(post_patch_command)
        
        check_call(
            args = command + [
                '--prefix=' + path_to_stagedir,
                '--disable-dependency-tracking',
                '--disable-shared',
                '--enable-static',
                ] + extra_args,
            env = get_updated_env(extra_compile_flags,
                                  extra_cflags,
                                  extra_cxxflags,
                                  extra_ldflags),
            )
        
        check_call(['/usr/bin/make', '-j', num_cores, 'install'])


################################################################################
#
# Library builders
#
################################################################################


@builder
def boost():
    version = '1.60.0'
    srcdir = 'boost_' + version.replace('.', '_')
    tarfile = srcdir + '.tar.bz2'

    download_archive_from_sf('boost/boost', version, tarfile)
    unpack_tarfile(tarfile, srcdir)

    with workdir(srcdir):
        apply_patch('boost_check_macro.patch')
        apply_patch('boost_ice_deprecations.patch')

        os.symlink('boost', 'mworks_boost')
        
        check_call([
            './bootstrap.sh',
            '--with-toolset=clang',
            ('--with-libraries='
             'filesystem,python,random,regex,serialization,system,thread'),
            '--without-icu',
            '--with-python=' + python,
            '--prefix=' + path_to_stagedir,
            ])
        
        check_call([
            './b2',
            #'-d', '2',  # Show actual commands run,
            '-j', num_cores,
            'optimization=space',
            'debug-symbols=on',
            'inlining=on',
            'runtime-debugging=off',
            'link=static',
            'threading=multi',
            'define=boost=mworks_boost',
            'cflags=' + compile_flags,
            'cxxflags=' + cxxflags,
            'linkflags=' + link_flags,
            'install',
            ])

    with workdir(path_to_stagedir + '/include'):
        if not os.path.islink('mworks_boost'):
            os.symlink('boost', 'mworks_boost')


@builder
def zeromq():
    version = '4.1.4'
    srcdir = 'zeromq-' + version
    tarfile = srcdir + '.tar.gz'

    download_archive('http://download.zeromq.org/', tarfile)
    unpack_tarfile(tarfile, srcdir)

    run_configure_and_make(
        srcdir = srcdir,
        extra_args = ['--disable-silent-rules', '--without-libsodium'],
        extra_ldflags = '-lc++',
        )


@builder
def libpng():
    version = '1.2.56'
    srcdir = 'libpng-' + version
    tarfile = srcdir + '.tar.gz'

    download_archive_from_sf('libpng/libpng12', version, tarfile)
    unpack_tarfile(tarfile, srcdir)

    run_configure_and_make(srcdir)


@builder
def jpeg():
    version = '9b'
    srcdir = 'jpeg-' + version
    tarfile = 'jpegsrc.v%s.tar.gz' % version

    download_archive('http://www.ijg.org/files/', tarfile)
    unpack_tarfile(tarfile, srcdir)

    run_configure_and_make(srcdir)


@builder
def tiff():
    version = '3.9.7'
    srcdir = 'tiff-' + version
    tarfile = srcdir + '.tar.gz'

    download_archive('http://download.osgeo.org/libtiff/', tarfile)
    unpack_tarfile(tarfile, srcdir)

    run_configure_and_make(
        srcdir = srcdir,
        extra_args = ['--disable-cxx', '--with-apple-opengl-framework'],
        )


@builder
def devil():
    version = '1.7.8'
    srcdir = 'devil-' + version
    tarfile = 'DevIL-%s.tar.gz' % version

    download_archive_from_sf('openil/DevIL', version, tarfile)
    unpack_tarfile(tarfile, srcdir)

    run_configure_and_make(
        srcdir = srcdir,
        extra_args = ['--enable-ILU', '--enable-ILUT', '--disable-exr',
                      '--disable-lcms', '--disable-mng', '--disable-utx',
                      '--without-squish', '--without-nvtt'],
        extra_compile_flags = ('-I%s/usr/include/malloc -I%s/include' %
                               (os.environ['SDKROOT'], path_to_stagedir)),
        extra_cflags = '-std=gnu89',
        extra_ldflags = ('-L%s/lib' % path_to_stagedir),
        )


@builder
def cppunit():
    version = '1.13.2'
    srcdir = 'cppunit-' + version
    tarfile = srcdir + '.tar.gz'

    download_archive('http://dev-www.libreoffice.org/src/', tarfile)
    unpack_tarfile(tarfile, srcdir)

    run_configure_and_make(srcdir, extra_compile_flags='-g0')


@builder
def numpy():
    # NOTE: We need to use the version of numpy that's distributed with
    # MACOSX_DEPLOYMENT_TARGET
    version = {
        '10.9': '1.6.2',
        '10.10': '1.8.0rc1',
        '10.11': '1.8.0rc1',
        }[os.environ['MACOSX_DEPLOYMENT_TARGET']]
    srcdir = 'numpy-' + version
    tarfile = srcdir + '.tar.gz'

    download_archive_from_sf('numpy/NumPy', version, tarfile)
    unpack_tarfile(tarfile, srcdir)

    with workdir(srcdir):
        env = get_updated_env()
        del env['MACOSX_DEPLOYMENT_TARGET']
        env['NPY_NUM_BUILD_JOBS'] = num_cores

        check_call([
            python,
            'setup.py',
            'install',
            '--install-lib=install',
            '--install-scripts=install',
            ],
            env=env)

        check_call([
            rsync,
            '-a',
            'install/numpy/core/include/numpy',
            path_to_stagedir + '/include',
            ])


@builder
def matlab_xunit():
    version = '3.1.1'
    srcdir = 'matlab_xunit_' + version
    zipfile = srcdir + '.zip'

    download_file(('http://www.mathworks.com/matlabcentral/fileexchange/'
                   'submissions/22846/v/13/download/zip'),
                  zipfile)
    unpack_zipfile(zipfile, srcdir)

    with workdir(srcdir):
        check_call([
            rsync,
            '-a',
            'matlab_xunit_3_1_1/xunit',
            path_to_stagedir + '/MATLAB',
            ])


@builder
def narrative():
    version = '0.1.2'
    srcdir = 'Narrative-' + version
    zipfile = srcdir + '.zip'

    download_archive_from_sf('narrative/narrative',
                             urllib.quote(srcdir.replace('-', ' ')),
                             zipfile)
    unpack_zipfile(zipfile, srcdir)

    with workdir('/'.join([srcdir, srcdir, 'Narrative'])):
        check_call([
            xcodebuild,
            '-project', 'Narrative.xcodeproj',
            '-configuration', 'Release',
            '-xcconfig', os.environ['MW_XCODE_DIR'] + '/Development.xcconfig',
            'clean',
            'build',
            'INSTALL_PATH=@loader_path/../Frameworks',
            'SDKROOT=%s' + os.environ['SDKROOT'],
            'SDKROOT_i386=$(SDKROOT)',
            'OTHER_CFLAGS=-fvisibility=default',
            ])

        check_call([
            rsync,
            '-a',
            'build/Release/Narrative.framework',
            path_to_stagedir + '/Frameworks',
            ])


################################################################################
#
# Main function
#
################################################################################


def main():
    if os.environ['ACTION'] == 'clean':
        return

    requested_builders = sys.argv[1:]
    builder_names = set(buildfunc.__name__ for buildfunc in all_builders)

    for name in requested_builders:
        if name not in builder_names:
            announce('ERROR: unknown builder: %r', name)
            sys.exit(1)

    if not requested_builders:
        remove_directory(stagedir)
    check_call(['/bin/mkdir', '-p', downloaddir, builddir, stagedir])

    with workdir(builddir):
        for buildfunc in all_builders:
            if ((not requested_builders) or
                (buildfunc.__name__ in requested_builders)):
                buildfunc()

    # Remove unwanted build products
    for dirpath in ('bin', 'lib/pkgconfig', 'share'):
        remove_directory(stagedir + '/' + dirpath)
    check_call(['/usr/bin/find', (stagedir + '/lib'), '-name', '*.la',
                '-exec', '/bin/rm', '{}', ';'])

    # Install files
    check_call([rsync, '-a', (stagedir + '/'), os.environ['MW_DEVELOPER_DIR']])


if __name__ == '__main__':
    main()
