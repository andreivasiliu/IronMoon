-- Imperian driver.

map_file = 'IMap'
title_color = C.m

map.destroy( )

if map.load(map_file) then
    debug( "Map loaded from '" .. map_file .. "'; driver: Imperian." )
    map.mode = "get_unlost";
end

--map.load_settings("config.mapper.txt")


function parse_room( )
    local title, offset
    local room
    
    if #queue == 0 and map.mode ~= "get_unlost" then
        return
    end
    
    for line = 1, mb.nr_of_lines do
        set_line( line )

        offset, title = string.match( mb.line, "In the trees above ()(.*%.)" )
        if not offset then
            offset, title = 1, mb.line
        end
        offset = offset - 1

        title = string.match( title, "(.*) %(road%)%." ) or title
        title = string.match( title, "(.*) %(path%)%." ) or title
        
        if ( get_color_at( offset ) == C.m ) and mb.len - offset > 0 then
            local dir = table.remove( queue, 1 )
            
            if map.mode == "following" and dir then
                room = map.locate_nearby( title, dir )
                if room then
                    map.current_room = room
                else
                    suffix( C.R .. " (" .. C.G .. "lost" .. C.R .. ")" .. C.x )
                    map.mode = "get_unlost"
                    map.current_room = nil
                end
            end
            
            if map.mode == "get_unlost" then
                local matches
                
                room, matches = map.locate_faraway( title )

                if room then
                    map.current_room = room
                    if matches > 1 then
                        prefix( -1, C.R .. "[Match probability: " ..
                                math.floor(100 / matches) .. "%]\n" .. C.x )
                        suffix( " (?)" )
                    end

                    map.mode = "following"
                end
            end

            if map.current_room then
                suffix( C.R .. " (" .. C.g .. map.current_room.area.name ..
                        C.R .. ")" .. C.x )
            
                if autowalking then
                    set_line( -1 )
                    go_next( suffix )
                end
            end
        end
    end
end


queue_blocks =
{
    "You cannot move that fast, slow down!",
    "You cannot move until you have regained equilibrium.",
    "You are regaining balance and are unable to move.",
    "You'll have to swim to make it through the water in that direction.",
    "rolling around in the dirt a bit.",
    "You are asleep and can do nothing. WAKE will attempt to wake you.",
    "There is a door in the way.",
    "You must first raise yourself and stand up.",
    "You are surrounded by a pocket of air and so must move normally through water.",
    "You cannot fly indoors.",
    "You slip and fall on the ice as you try to leave.",
    "A rite of piety prevents you from leaving.",
    "A wall blocks your way.",
    "The hands of the grave grasp at your ankles and throw you off balance.",
    "You are unable to see anything as you are blind.",
    "The forest seems to close up before you and you are prevented from moving that ",
    "There is no exit in that direction.",
    "The ground seems to tilt and you fall with a thump.",
    "Your legs are tangled in a mass of rope and you cannot move.",
    "You must stand up to do that.",
    "Your legs are crippled, how will you move?",
    "You cannot walk through an enormous stone gate.",
    "You must regain your equilibrium first.",
    "You must regain balance first.",
    "You fumble about drunkenly.",
    "You try to move, but the quicksand holds you back.",
    "You struggle to escape the quicksand, but only manage to partly free yourself.",
    "A loud sucking noise can be heard as you try desperately to escape the mud.",
    "You struggle to escape the mud, but only manage to partly free yourself.",
    "A strange force prevents the movement.",
}

-- Turn the nr->string into string->true
for i, v in ipairs(queue_blocks) do
    queue_blocks[v] = true
end

-- And delete the former ones.
while #queue_blocks > 0 do
    table.remove( queue_blocks )
end


function parse_blocks( )
    for line = 1, mb.nr_of_lines do
        set_line( line )
        if queue_blocks[mb.line] then
            table.remove( queue, 1 )
        end

        if autowalking and mb.line == "You cannot move that fast, slow down!" then
            set_line( -1 )
            go_next( suffix )
        end
    end
end


function parse_dash( )
    local dir, start_line, room
    
    for line = 1, mb.nr_of_lines do
        set_line( line )
        
        dir = string.match( mb.line, "You look off to the (.*) and dash speedily away." )

        if dir and dir_names[dir] then
            start_line = line + 1
            break
        end
    end
    
    if dir then
        for line = start_line, mb.nr_of_lines do
            set_line( line )
            
            room = string.match( mb.line, "You dash through (.*%.)" )

            if room then
                local r = map.locate_nearby( room, dir )

                if r then
                    map.current_room = r
                    suffix( C.D .. " (" .. C.G .. r.vnum .. C.D .. ")" .. C.x )
                else
                    map.current_room = nil
                    map.mode = "get_unlost"
                    suffix( C.D .. " (" .. C.G .. "lost" .. C.D .. ")" .. C.x )
                    break
                end
            else
                break
            end
        end
    end
end


function mb.server_lines( )
    parse_dash( )
    
    parse_room( )

    parse_blocks( )
end


-- This is THE Queue. It needs no other name.
queue = { }


dir_names =
{
   n = true, ne = true, e = true, se = true, s = true, sw = true,
   w = true, nw = true, north = true, northeast = true, east = true,
   southeast = true, south = true, southwest = true, west = true,
   northwest = true, ["in"] = true, out = true, u = true, up = true,
   d = true, down = true
}


function can_dash( dir )
    local room
    
    room = map.current_room
    
    -- Check if we have more than one room.
    if not room[dir] or not room[dir][dir] then
        return false
    end

    -- Now check if we have a clear way, 'till the end.
    while room do
        if room[dir] then
            if room.pf_command ~= dir then
                return false
            elseif room[dir].environment.must_swim then
                return false
            end
        end

        room = room[dir]
    end
    
    return true
end


function must_swim( src, dest )
    if not src or not dest then
        return false
    end
    
    -- check for config 'disable_swimming'.. and some pear defence thingy.

    if src.environment.must_swim or dest.environment.must_swim then
        return true
    end

    return false
end


function go_next( echofunc )
    local here = map.current_room
    local room, count
    local cmd
    
    if not here then
        echofunc( C.R .. "[I've no idea where I am!]" .. C.x )
        autowalking = false
        return
    end

    if not here.pf_next then
        echofunc( C.R .. "(" .. C.G .. "Done." .. C.R .. ") " .. C.x )
        autowalking = false
        return
    end
    
    if not here.pf_command then
        echofunc( C.R .. "(" .. C.r .. "waiting.." .. C.R .. ") " .. C.x )
        return
    end

    count, room = 0, here.pf_next
    while room do
        count = count + 1
        room = room.pf_next
    end

    cmd = here.pf_command

    if dir_names[cmd] then
        table.insert( queue, cmd )
        add_timer( 10, queue_reset, "lua_queue_reset_timer" )
        
        if must_swim( here, here.pf_next ) then
            cmd = "swim " .. cmd
        end

        if can_dash( cmd ) then
            cmd = dash_command .. " " .. cmd
        end
    end
    
    send( cmd )
    echofunc( C.R .. "(" .. C.r .. count .. " - " .. C.R .. cmd .. C.R .. ") " .. C.x )
end

function queue_reset( )
    if #queue > 0 then
        echo( "Queue reset!" )
        show_prompt( )
        queue = { }
    end
end

function mb.client_aliases( cmd )
    local movement
    local spexit
    
    if cmd == "go" then
        autowalking = true
        dash_command = nil
        go_next( echo )
        return true
    elseif cmd == "go dash" then
        autowalking = true
        dash_command = "dash"
        go_next( echo )
        return true
    end
    
    if cmd == "stop" and autowalking then
        autowalking = false
        echo( "Okay, I'm frozen." )
        show_prompt( )
        return true
    end
    
    if cmd == 'l' or cmd == 'look' or cmd == 'ql' then
        table.insert( queue, "look" )
        add_timer( 10, queue_reset, "lua_queue_reset_timer" )
    end
    
    -- Lua-fu
    movement = string.match( cmd, "swim (.*)" ) or
               string.match( cmd, "evade (.*)" ) or
               string.match( cmd, "dash (.*)" ) or
               string.match( cmd, "sprint (.*)" ) or
               string.match( cmd, "wolfvault (.*)" ) or
               cmd

    if dir_names[movement] then
        table.insert( queue, movement )
        add_timer( 10, queue_reset, "lua_queue_reset_timer" )
    end
end

