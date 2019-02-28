create or replace procedure register_account (
    in in_email         varchar(128),
    in in_password_hash varchar(256)
)
language plpgsql
as $$
begin
    insert into account (email, password_hash)
         values (in_email, in_password_hash);
end;
$$;
