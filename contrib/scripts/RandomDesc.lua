---   RandomDesc.lua for KwaKwa     ---
---         written by              ---
---   Filip "widelec" Maryjanski    ---
---      widelec@morphos.pl         ---
---        public domain            ---
--- usage: LuaX RandomDesc.lua file ---

require('base');
require('io');
require('table');
require('math');
require('ipc');


if arg[1] == nil
then
	print('Usage: LuaX RandomDesc.lua file_name');
	return;
end

address('KWAKWA.1')

status = {'available', 'away', 'invisible', 'ffc', 'dnd'};
lines = {};

for line in io.lines(arg[1])
do
	table.insert(lines, line);
end

lines_no = # lines;
random_line = lines[math.random(lines_no)];
random_status = status[math.random(5)];

result = rx('ChangeStatus', random_status, random_line);

if result == 'OK'
then
	print('Done.');
elseif result == 'Failed'
then
	print('Error!');
elseif result == 'Wrong status name'
then
	print('Wrong status name!');
end
