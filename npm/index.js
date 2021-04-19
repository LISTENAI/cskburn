const { join } = require('path');
const { existsSync } = require('fs');
const { arch, platform } = process;

exports.CSKBURN = (() => {
    const suffix = (platform == 'win32') ? '.exe' : '';
    const binary = join(__dirname, `cskburn-${platform}-${arch}${suffix}`);
    if (existsSync(binary)) {
        return binary;
    } else {
        return null;
    }
})();
