function animateScanning(Name,dataPath,exportLocation)
%ANIMATEREGISTRATION
%  This function creates a animation showing the initial and final
%  poses for a set of surfaces algned with a registration. The
%  animation orbits around the two models over 15 seconds.
%  __________________________________________________________________
%  ANIMATEREGISTRATION()
%       Runs the animation for the partial bunny both as initial and
%		final pose.
%       '../data/' and exports the result in the folder
%       '../logs/matlab'.
%
%  ANIMATEREGISTRATION(nameIni,nameEnd)
%       Read the initial and final poses from the file names specified.
%       Import and export path is as above.
%
%  ANIMATEREGISTRATION(name, dataPath)
%       Locates the models in the folder specified by 'dataPath'.
%
%  ANIMATEREGISTRATION(name, dataPath, exportLocation)
%       Exports the animation at location specified by
%		'exportLocation'.
%1
%  See also EXPORTFIGURES.

%% Handle input
if ~exist('Name','var') || isempty(Name)
    Name = 'bunnyPartial';
end
if ~exist('dataPath','var') || isempty(dataPath)
    dataPath = '../data/';
end
if ~exist('exportLocation','var') || isempty(dataPath)
    exportLocation = '../logs/matlab';
end
dataName = findData(dataPath,Name);
if isempty(dataName)
    error('File %s not found.',Name);
end

if ~strcmp(dataPath(end),'/')
	dataPath = [dataPath,'/'];
end
%% Generate the correspondence plots
if (size(dataName,2) == 2)
    Color = flipud(colormap(gray(3)));
else
    Color = flipud(colormap(white(max(size(dataName,2),size(dataName,2)))));
end

F = CreateFigure('AnimationScanning');
F.WindowStyle = 'normal';
F.Position = [50,50,1000,1000];
F.Color = [1,1,1];
A = axes();
axis tight
set(A,'DataAspectRatio',[1,1,1]);
axis vis3d

set(A,'Projection','perspective')
set(A,'CameraPosition',get(A(1),'CameraPosition'));

% Find the axes in question
v = VideoWriter([exportLocation,'/','aniScan'],'Uncompressed AVI');

% Settings for the video
v.FrameRate = 20;
Time = 15;


Frames = Time*v.FrameRate;
framePerModel = Frames/length(dataName);
open(v);
count = -1;
id_old = 0;

for i = 0 : Frames-1
    id = floor(i/framePerModel)+1;
    
    if id ~= id_old
        id_old = id;
        count = 1;
    else 
        count = count + 1;
    end
    
    per1 = count/(v.FrameRate*0.25);
    per2 = 1 - (count)/(v.FrameRate*0.25);
    
    if per1 <= 1.0
        dispReg(dataName{id},dataPath,Color(id,:),per1);
    end
    if per2 > 0.0 && id > 1
        hold on
        dispReg(dataName{id-1},dataPath,Color(id_old,:),per2);
        hold off
    end
    
    view([90,20])
    if i == 0
        Cam = A.CameraPosition;
        Lim = [A.XLim;A.YLim;A.ZLim];
    end
    
    A.XLim = Lim(1,:);
    A.YLim = Lim(2,:);
    A.ZLim = Lim(3,:);
    A.CameraPosition = Cam;
    drawnow;
    
    frame = getframe(F);
    writeVideo(v,frame);
    if i == 0
        ExportFigures(F,exportLocation);
    end
end
close(v);
close(F);
end

function dispReg(name,dataPath,Color,per)
data = name ;
if ~exist([dataPath,data],'file')
    return;
end

%% Load the data
model = pcread([dataPath,data]);
model = pcdownsample(model,'random',per);
normal = pcnormals(model);


L = [0,1,1];
ambient = 0.1;
L = L./norm(L);
I = normal(:,1).*L(1) + normal(:,2).*L(2) + normal(:,3).*L(3);
I = abs(I)*(1.0-ambient) + ambient;

S = scatter3(model.Location(:,1),model.Location(:,2),model.Location(:,3),3);
S.CData = I*Color;
S.Marker = 'O';
S.MarkerEdgeColor = 'flat';
S.MarkerFaceColor = 'flat';

hold off
axis off

end