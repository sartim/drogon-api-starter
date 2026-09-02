INSERT INTO public.users
    (first_name, last_name, email, password, is_deleted)
VALUES
    ('Integration', 'Fixture', 'integration@example.test',
     '$2a$10$N9qo8uLOickgx2ZMRZoMyeIjZAgcfl7p92ldGxad68LJZdL17lhWy', FALSE)
ON CONFLICT (email) DO UPDATE
SET password = EXCLUDED.password,
    is_deleted = FALSE;
