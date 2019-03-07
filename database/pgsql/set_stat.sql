create or replace procedure set_stat (
    in in_player_id  int,
    in in_stat_type  int,
    in in_experience int
)
language plpgsql
as $$
begin
    insert into stat (player_id, stat_type, experience)
         values (in_player_id, in_stat_type, in_experience)
    on conflict 
        on constraint pk_stat
            do update
                  set experience = in_experience;
end;
$$;
