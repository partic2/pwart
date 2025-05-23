import subprocess
import os

def url_replacer(url):
    return url

git='git'

deps_dir=os.environ.get('DEPS_SOURCE_DIRS',os.path.dirname(os.path.abspath(__file__)))

def pull(dep_name, repo_url, branch="main"):
    """
    Pull remote repository dependency to local
    Args:
        dep_name (str): Dependency name (will be used as directory name)
        repo_url (str): Repository URL (can be replace by url_replacer, if needed)
        branch (str): Repository branch, defaults to main
    """

    repo_url = url_replacer(repo_url)

    base_dir = deps_dir
    target_dir = os.path.join(base_dir, dep_name)
    
    if os.path.exists(target_dir):
        try:
            # Execute git clone command
            cmd = [
                git, 'pull',
                '--rebase'
            ]
            subprocess.run(cmd, check=True,cwd=target_dir)
            print(f"[OK] Successfully pulled {dep_name} @{branch}")
            
        except subprocess.CalledProcessError as e:
            print(f"[ERROR] Failed to pull {dep_name}: {str(e)}")
        except Exception as e:
            print(f"[ERROR] Unexpected error: {str(e)}")
    else:
        try:
            # Execute git clone command
            cmd = [
                git, 'clone',
                '-b', branch,
                '--depth', '1',
                repo_url,
                target_dir
            ]
            subprocess.run(cmd, check=True)
            print(f"[OK] Successfully pulled {dep_name} @{branch}")
            
        except subprocess.CalledProcessError as e:
            print(f"[ERROR] Failed to pull {dep_name}: {str(e)}")
        except Exception as e:
            print(f"[ERROR] Unexpected error: {str(e)}")


def main():
    pull('libffi','https://gitee.com/partic/libffi','main')
    pull('libuv','https://gitee.com/partic/libuv-patched','v1.x')
    pull('uvwasi','https://gitee.com/partic/uvwasi','main')

if __name__=='__main__':
    main()
    