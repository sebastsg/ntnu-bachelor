create or replace procedure find_player_by_name (
    in in_display_name varchar(32)
)
language plpgsql
as $$
begin
    select *
      from player
     where display_name = in_display_name;
end;
$$;
