const { readFileSync, openSync, writeSync, closeSync } = require('fs');
const { join } = require('path');

const [input, output, name] = process.argv.slice(2);
if (!input || !output || !name) {
    console.log('Usage: node bin2c.js [input] [output] [name]');
    process.exit(-1);
}

const fd = openSync(output, 'w');

writeSync(fd, `#include <stdint.h>\n`);
writeSync(fd, `\n`);
writeSync(fd, `uint8_t ${name}[] = {\n`);

let buf = readFileSync(input, { encoding: null });
while (buf.length > 0) {
    const line = Array.prototype.slice.apply(buf.slice(0, 16))
        .map(b => `0x${b.toString(16).padStart(2, '0')}`)
        .join(', ');

    writeSync(fd, `\t${line},\n`);

    buf = buf.slice(16);
}

writeSync(fd, `};\n`);
writeSync(fd, `\n`);
writeSync(fd, `uint32_t ${name}_len = sizeof(${name});\n`);
closeSync(fd);
