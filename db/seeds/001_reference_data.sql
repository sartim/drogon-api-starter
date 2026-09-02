INSERT INTO public.roles (name, description)
VALUES ('admin', 'Full service administrator')
ON CONFLICT (name) DO UPDATE
SET description = EXCLUDED.description,
    updated_at = CURRENT_TIMESTAMP,
    is_deleted = FALSE;

INSERT INTO public.permissions (name, description)
VALUES
    ('users:read', 'Read users'),
    ('users:write', 'Create and update users'),
    ('roles:read', 'Read roles'),
    ('roles:write', 'Create and update roles')
ON CONFLICT (name) DO UPDATE
SET description = EXCLUDED.description,
    updated_at = CURRENT_TIMESTAMP,
    is_deleted = FALSE;
