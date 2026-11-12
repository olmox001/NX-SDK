# USERKIT service extensions

This directory is the SDK-facing extension plane for NX-USERKIT. Each service project has one extension directory containing only its public interface, reusable client-side implementation, layout/model code when shared by clients, and project metadata.

Server entrypoints are not duplicated here. They live only in the NX-USERKIT repository under `services/<project>/src/main.c`. Generic object and graphics adapters remain under `lib/extensions/object` and `lib/extensions/graphics`.
