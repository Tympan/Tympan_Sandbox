%% function CHAPRO_DFS(param,val,com_num,baudrate)
% Select '-' for param to send single character commands.  For example, to mute use '-' for param and 'z' for val

function CHAPRO_DFS(param,val,com_num,baudrate)

if nargin < 4
    baudrate = 115200;
end

switch param
    case 'gain'
        v = num2hex(single(val));
        hexData(1:11,:) = ['02'; '09'; '00'; '00'; '00'; '03'; '67'; '61'; '69'; '6E'; '03'];
        hexData(12,:) = v(7:8); hexData(13,:) = v(5:6); hexData(14,:) = v(3:4); hexData(15,:) = v(1:2);
        hexData(16,:) = '04';

    case 'mu'
        v = num2hex(single(val));
        hexData(1:9,:) = ['02'; '07'; '00'; '00'; '00'; '03'; '6d'; '75'; '03'];
        hexData(10,:) = v(7:8); hexData(11,:) = v(5:6); hexData(12,:) = v(3:4); hexData(13,:) = v(1:2);
        hexData(14,:) = '04';

    case 'eps'
        v = num2hex(single(val));
        hexData(1:10,:) = ['02'; '08'; '00'; '00'; '00'; '03'; '65'; '70'; '73'; '03'];
        hexData(11,:) = v(7:8); hexData(12,:) = v(5:6); hexData(13,:) = v(3:4); hexData(14,:) = v(1:2);
        hexData(15,:) = '04';

    case 'rho'
        v = num2hex(single(val));
        hexData(1:10,:) = ['02'; '08'; '00'; '00'; '00'; '03'; '72'; '68'; '6F'; '03'];
        hexData(11,:) = v(7:8); hexData(12,:) = v(5:6); hexData(13,:) = v(3:4); hexData(14,:) = v(1:2);
        hexData(15,:) = '04';

    case '-'
        SID = serialport(sprintf('COM%s',num2str(com_num)),baudrate);
        fwrite(SID,val);
        SID = [];
end

if param ~= '-'
    SID = serialport(sprintf('COM%s',num2str(com_num)),baudrate);
    fwrite(SID,hex2dec(hexData),'uint8');
    SID = [];
end