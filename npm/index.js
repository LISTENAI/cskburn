const { platform } = require('os');
const { join } = require('path');
exports.CSKBURN = (() => {
    switch (platform()) {
        case 'darwin':
            return join(__dirname, 'cskburn-darwin');
        case 'linux':
            return join(__dirname, 'cskburn-linux');
        case 'win32':
            return join(__dirname, 'cskburn-win32.exe');
        default:
            return null;
    }
})();
