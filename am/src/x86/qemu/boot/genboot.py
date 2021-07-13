import os, sys, pathlib, subprocess

f = pathlib.Path(sys.argv[1])
try:
    objcopy = os.getenv('CROSS_COMPILE', '') + 'objcopy'
    data = subprocess.run(
        [objcopy, '-S', '-O', 'binary', '-j', '.text', f, '/dev/stdout'],
        capture_output=True).stdout
    assert len(data) <= 510
    data += b'\0' * (510 - len(data)) + b'\x55\xaa'
    f.write_bytes(data)
except:
    f.unlink()
    raise
