const { join } = require('path');
const { existsSync } = require('fs');
const { family, MUSL } = require('detect-libc');
const { arch, platform } = process;

exports.CSKBURN = (() => {
    const suffix = (platform == 'win32') ? '.exe' : '';
    const libc = (platform == 'linux' && family == MUSL) ? '-musl' : '';
    const binary = join(__dirname, `cskburn-${platform}-${arch}${libc}${suffix}`);
    if (existsSync(binary)) {
        return binary;
    } else {
        return null;
    }
})();
