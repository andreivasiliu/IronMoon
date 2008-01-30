-- Imperian driver.

map_file = 'IMap'
title_color = C.m


if map.load(map_file) then
    debug( "Map loaded from '" .. map_file .. "'; driver: Imperian." )
end

--map.load_settings("config.mapper.txt")


function parse_room( )
    local title
    
    for line = 1, mb.nr_of_lines do
        set_line( line )
        
        if ( get_color_at( 1 ) == C.m ) then
            suffix( ' (line)' )
            map.parse_title( mb.line )
        end
    end
    

end


function mb.server_lines( )
    parse_room( )
end

