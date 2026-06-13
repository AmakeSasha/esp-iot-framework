import subprocess
import sys
import os

def callback_cppcheck(action=None, ctx=None, args=None):
    print("Running `cppcheck` (MISRA C)...")
    
    cwd = os.getcwd()
    
    config_dir = cwd
    while config_dir and not os.path.exists(os.path.join(config_dir, "esp_iot_framework.cppcheck")):
        parent = os.path.dirname(config_dir)
        if parent == config_dir:
            break
        config_dir = parent

    os.makedirs(f"{config_dir}/esp_iot_framework_cppcheck_build_dir", exist_ok=True)
    
    command = [
        "cppcheck",
        f"--project={config_dir}/esp_iot_framework.cppcheck",
        # "--addon=misra",
        "-I", "build/config"
    ]
    
    try:
        res = subprocess.run(command, check=True, text=True, cwd=config_dir)
        sys.exit(res.returncode)
    except subprocess.CalledProcessError as e:
        print(f"`cppcheck` failed with exit code: {e.returncode}")
        sys.exit(e.returncode)
    except FileNotFoundError:
        print("Error: 'cppcheck' not found.")
        sys.exit(1)

def action_extensions(base_actions, project_path):
    return {
        "actions": {
            "cppcheck": {
                "callback": callback_cppcheck,
                "help": "Run `cppcheck` MISRA C analysis using framework config",
            }
        }
    }

if __name__ == "__main__":
    callback_cppcheck()