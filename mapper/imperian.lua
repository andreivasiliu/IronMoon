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
    local title
    local room
    
    for line = 1, mb.nr_of_lines do
        set_line( line )

        if ( get_color_at( 0 ) == C.m ) and mb.len ~= 0 then
            local dir = table.remove( queue, 1 )
            
            if map.mode == "following" and dir then
                room = map.locate_nearby( mb.line, dir )
                if room then
                    map.current_room = room
                    suffix( " (walking)" )
                else
                    suffix( " (lost?)" )
                    map.mode = "get_unlost"
                end
            end

            if map.mode == "get_unlost" then
                local matches
                
                room, matches = map.locate_faraway( mb.line )

                if room then
                    map.current_room = room
                    if matches > 1 then
                        suffix( "(?)" )
                    end

                    map.mode = "following"
                end
            end

            if map.current_room then
                suffix( " (walking [" .. map.current_room.vnum .. "])" )
            end
        end
    end
end


function mb.server_lines( )
    parse_room( )
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

function mb.client_aliases( cmd )
    local movement

    if ( cmd == 'l' or cmd == 'look' or cmd == 'ql' ) then
        table.insert( queue, "look" )
    end

    -- Lua-fu
    movement = string.match( cmd, "swim (.*)" ) or
               string.match( cmd, "evade (.*)" ) or
               string.match( cmd, "wolfvault (.*)" ) or
               cmd

    if dir_names[movement] then
        function queue_reset( )
            if #queue > 0 then
                echo( "Queue reset!" )
                show_prompt( )
                queue = { }
            end
        end
        table.insert( queue, movement )
        add_timer( 10, queue_reset, "lua_queue_reset_timer" )
    end
end

