create or replace procedure find_player_by_email (
    in in_email varchar(128)
)
language plpgsql
as $$
begin
    select *
      from player
     where email = in_email;
end;
$$;
