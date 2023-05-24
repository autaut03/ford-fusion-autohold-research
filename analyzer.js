import { SerialPort } from 'serialport';
import { ReadlineParser } from '@serialport/parser-readline';

const port = new SerialPort({
    path: '/dev/cu.wchusbserial110',
    // path: '/dev/cu.usbserial-110',
    baudRate: 115200,
});
const lineStream = port.pipe(new ReadlineParser({ delimiter: '\n' }));

const state = new Map();
let lastChangedToOn = null;
let notIt = [];

const datedLog = function (...args) {
    const now = new Date;

    console.log('[' + now.toLocaleTimeString('it-IT') + '.' + now.getMilliseconds() + '] ', ...args);
};
function hex2bin(hex){
    return (parseInt(hex, 16).toString(2)).padStart(8, '0');
}

port.on("open", () => {
    datedLog('serial port open');
});
lineStream.on('data', input => {
    datedLog('>>>> ' + input);
    return;

    if (input.includes('Emitting button state as ON')) {
        lastChangedToOn = new Date();
    }

    if (!input.startsWith('#')) {
        datedLog('>>>> ' + input);

        return;
    }

    const [canId, dataString] = input.substring(1).split(':');
    const data = dataString.split(',');

    data
        .flatMap(hex => hex2bin(hex).split(''))
        .forEach((dataByte, index) => {
            const indexString = index.toString();
            const byteIdentifier = canId + ':' + indexString;
            const now = new Date();

            if (!state.has(byteIdentifier)) {
                datedLog('>>> SET: ' + canId + ' > b' + indexString + ': ' + dataByte);

                state.set(byteIdentifier, {
                    state: dataByte,
                    changed: now,
                });

                return;
            }

            const { state: previousState, changed: previouslyChangedAt } = state.get(byteIdentifier);

            if (previousState === dataByte || notIt.includes(byteIdentifier)) {
                return;
            }

            state.set(byteIdentifier, {
                state: dataByte,
                changed: now,
            });

            if (lastChangedToOn === null) {
                notIt.push(byteIdentifier);

                datedLog('>>> NOT it: ' + canId + ' > b' + indexString + ': ' + previousState + ' -> ' + dataByte);

                return;
            }

            datedLog('>>> Possibly it: ' + canId + ' > b' + indexString + ': ' + previousState + ' -> ' + dataByte);
        });
});
