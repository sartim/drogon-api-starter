CREATE EXTENSION IF NOT EXISTS pgcrypto;

INSERT INTO public.users
    (first_name, last_name, email, password, is_deleted)
VALUES
    ('Integration', 'Fixture', 'integration@example.test',
     crypt('integration-password', gen_salt('bf', 10)), FALSE)
ON CONFLICT (email) DO UPDATE
SET password = EXCLUDED.password,
    is_deleted = FALSE;
