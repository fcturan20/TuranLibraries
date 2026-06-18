import argparse
import os
import platform
import shutil
import sys
import subprocess

vcpkg_toolchain = None
vcpkg_executable = None


def get_default_triplet():
    env_triplet = os.getenv('VCPKG_TARGET_TRIPLET')
    if env_triplet:
        return env_triplet

    system = platform.system()
    machine = (platform.machine() or '').lower()
    is_arm64 = machine in ('arm64', 'aarch64')

    if system == 'Windows':
        return 'arm64-windows' if is_arm64 else 'x64-windows'
    if system == 'Darwin':
        return 'arm64-osx' if is_arm64 else 'x64-osx'
    if system == 'Linux':
        return 'arm64-linux' if is_arm64 else 'x64-linux'
    return 'x64-windows'

# Given a vcpkg root directory, search for vcpkg.cmake file
def search_in_vcpkg_root(vcpkg_root):
    potential_path = os.path.join(vcpkg_root, 'scripts', 'buildsystems', 'vcpkg.cmake')
    if os.path.isfile(potential_path):
        return potential_path

    for root, dirs, files in os.walk(vcpkg_root):
        if 'vcpkg.cmake' in files:
            return os.path.join(root, 'vcpkg.cmake')
        
    return None

# Try to find vcpkg toolchain file in python
def find_vcpkg_toolchain():
    global vcpkg_toolchain
    
    # Check if VCPKG_TOOLCHAIN_FILE environment variable is set
    potential_path = os.getenv('VCPKG_TOOLCHAIN_FILE')
    if potential_path and os.path.isfile(potential_path):
        vcpkg_toolchain = potential_path
        return True

    # Check if VCPKG_ROOT environment variable is set
    vcpkg_root = os.getenv('VCPKG_ROOT')
    if vcpkg_root:
        potential_path = search_in_vcpkg_root(vcpkg_root)
        if potential_path:
            vcpkg_toolchain = potential_path
            return True

    # If not found, search External/vcpkg folder
    external_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'External', 'vcpkg')
    if os.path.isdir(external_dir):
        potential_path = search_in_vcpkg_root(external_dir)
        if potential_path:
            vcpkg_toolchain = potential_path
            return True
    
    print("vcpkg toolchain file not found. Please set VCPKG_TOOLCHAIN_FILE or VCPKG_ROOT environment variable, or install vcpkg in External folder.")
    return False

# Install vcpkg to External folder if not found
def install_vcpkg():
    import subprocess
    import platform

    vcpkg_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'External', 'vcpkg')
    if not os.path.isdir(vcpkg_dir):
        print("Cloning vcpkg into External folder...")
        subprocess.run(['git', 'clone', 'https://github.com/microsoft/vcpkg.git', vcpkg_dir])
        print("Bootstrapping vcpkg...")
        bootstrap_script = 'bootstrap-vcpkg.bat' if platform.system() == 'Windows' else 'bootstrap-vcpkg.sh'
        subprocess.run([os.path.join(vcpkg_dir, bootstrap_script)], shell=True)
        print("vcpkg installed successfully.")
    else:
        print("vcpkg already exists in External folder.")

def check_vcpkg():
    global vcpkg_executable

    if not find_vcpkg_toolchain():
        print("vcpkg toolchain file not found. Installing vcpkg...")
        install_vcpkg()
        if not find_vcpkg_toolchain():
            print("Failed to find vcpkg toolchain file after installation. Please check the installation process.")
            sys.exit(1)

    if not vcpkg_toolchain:
        print("vcpkg toolchain file path is empty after discovery.")
        sys.exit(1)
    
    print("vcpkg toolchain file found at:", vcpkg_toolchain)
    vcpkg_root = os.path.abspath(os.path.join(os.path.dirname(vcpkg_toolchain), '..', '..'))
    vcpkg_exe_name = 'vcpkg.exe' if os.name == 'nt' else 'vcpkg'
    toolchain_executable = os.path.join(vcpkg_root, vcpkg_exe_name)

    if os.path.isfile(toolchain_executable):
        vcpkg_executable = toolchain_executable
    else:
        vcpkg_executable = shutil.which('vcpkg')

    if not vcpkg_executable:
        print("Could not locate vcpkg executable. Please bootstrap vcpkg or add it to PATH.")
        sys.exit(1)

def generate_project(args):
    if not vcpkg_toolchain:
        print("vcpkg toolchain path is not initialized.")
        sys.exit(1)

    # Use cross-platform architecture detection on Windows/Linux/macOS.
    platform_name = sys.platform
    architecture = platform.machine() or os.environ.get('PROCESSOR_ARCHITECTURE', 'unknown')

    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.abspath(os.path.join(script_dir, '..'))
    build_dir = os.path.join(project_root, 'Project', platform_name + '_' + architecture)
    toolchain_argument = '-DCMAKE_TOOLCHAIN_FILE=' + vcpkg_toolchain

    cmake_cmd = [
        'cmake',
        '-S', project_root,
        '-B', build_dir,
        toolchain_argument,
        '-DTCMAKE_INVOKER=regen.py'
    ]
    
    cmake_status = subprocess.run(cmake_cmd)
    if cmake_status.returncode != 0:
        print("CMake generation failed with return code:", cmake_status.returncode)
        sys.exit(1)

    print("CMake generation completed. You can now build the project using your preferred method (e.g., cmake --build build).")

def build_project():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.abspath(os.path.join(script_dir, '..'))
    platform_name = sys.platform
    architecture = platform.machine() or os.environ.get('PROCESSOR_ARCHITECTURE', 'unknown')
    build_dir = os.path.join(project_root, 'Project', platform_name + '_' + architecture)

    if not os.path.isdir(build_dir):
        print("Build directory does not exist. Please run the generation step first.")
        sys.exit(1)

    build_status = subprocess.run(['cmake', '--build', build_dir])
    if build_status.returncode != 0:
        print("Build failed with return code:", build_status.returncode)
        sys.exit(1)

    print("Build completed successfully.")

if __name__ == "__main__":
    check_vcpkg()

    parser = argparse.ArgumentParser(description="Regenerate project files using CMake and vcpkg.")
    # --gen command to generate CMake files
    # --install to install dependencies using vcpkg, usage is python regen.py --install glfw3 glm imgui will install these dependencies using vcpkg
    parser.add_argument('--gen', action='store_true', help='Generate CMake files using vcpkg toolchain')
    parser.add_argument('--install', nargs='+', help='Install dependencies using vcpkg (e.g., --install glfw3 glm imgui)')
    parser.add_argument('--triplet', default=get_default_triplet(), help='vcpkg triplet used for dependency installation')
    parser.add_argument('--build', action='store_true', help='Build the project after generating CMake files')
    args = parser.parse_args()

    if args.install:
        if not vcpkg_executable:
            print("vcpkg executable path is not initialized.")
            sys.exit(1)

        vcpkg_cwd = os.path.dirname(vcpkg_executable)
        for dep in args.install:
            dep_spec = f"{dep}:{args.triplet}" if ':' not in dep else dep
            print(f"Installing {dep_spec} using vcpkg...")
            install_status = subprocess.run([vcpkg_executable, 'install', dep_spec], cwd=vcpkg_cwd)
            if install_status.returncode != 0:
                print(f"Failed to install {dep} with return code:", install_status.returncode)
                sys.exit(1)
        print("All dependencies installed successfully.")

    if args.gen:
        generate_project(args)

    if args.build:
        build_project()