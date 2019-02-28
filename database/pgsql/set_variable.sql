create or replace procedure set_variable (
    in in_player_id int,
    in in_scope     int,
    in in_name      varchar(32),
    in in_value     varchar(64),
    in in_type      int
)
language plpgsql
as $$
begin
    insert into variable (player_id, scope, var_name, var_value, var_type)
         values (in_player_id, in_scope, in_name, in_value, in_type)
    on conflict 
        on constraint pk_variable
            do update
                  set var_value = in_value;
end;
$$;
