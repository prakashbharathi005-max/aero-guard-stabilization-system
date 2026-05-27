
s = serialport("COM4",9600);
pause(3);   % allow ESP to stabilize

angleData = [];
pidData = [];

figure;

while true
    
    if s.NumBytesAvailable > 0
        
        data = readline(s);
        data = strtrim(string(data));   % convert safely
        
        disp(data);   % DEBUG (see incoming data)
        
        % ✅ Skip empty or bad data
        if strlength(data) == 0
            continue;
        end
        
        % ✅ Only process valid CSV
        if contains(data, ",")
            
            values = str2double(split(data, ","));
            
            % ✅ Check valid numbers
            if numel(values) == 2 && all(~isnan(values))
                
                angle = values(1);
                pid = values(2);
                
                angleData(end+1) = angle;
                pidData(end+1) = pid;
                
                plot(angleData,'b','LineWidth',1.5); hold on;
                plot(pidData,'r','LineWidth',1.5);
                hold off;
                
                legend('Angle','PID Output');
                ylim([-90 90]);
                grid on;
                
                drawnow;
            end
        end
    end
end