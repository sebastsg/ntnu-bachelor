create or replace procedure register_player (
    in in_email         varchar(128),
    in in_display_name  varchar(32)
)
language plpgsql
as $$
begin
    insert into player (account_email, display_name)
         values (in_email, in_display_name);
end;
$$;
