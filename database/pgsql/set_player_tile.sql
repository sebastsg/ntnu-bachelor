create or replace procedure set_player_tile (
    in in_player_id int,
    in in_tile_x    int,
    in in_tile_z    int
)
language plpgsql
as $$
begin
    update player
       set tile_x = in_tile_x,
           tile_z = in_tile_z
     where id = in_player_id;
end;
$$;
