import pytest
import pytest_asyncio
from httpx import AsyncClient, ASGITransport
from sqlalchemy.ext.asyncio import create_async_engine, async_sessionmaker, AsyncSession
from sqlalchemy.pool import StaticPool

import app.database as db_module
from app.database import Base, get_db
from app.main import app

# Use in-memory SQLite for tests (no PostgreSQL or asyncpg needed)
TEST_DATABASE_URL = "sqlite+aiosqlite:///:memory:"


@pytest_asyncio.fixture(scope="function")
async def db_engine():
    engine = create_async_engine(
        TEST_DATABASE_URL,
        connect_args={"check_same_thread": False},
        poolclass=StaticPool,
    )
    async with engine.begin() as conn:
        await conn.run_sync(Base.metadata.create_all)
    # Patch module-level globals so get_engine() / get_session_factory() use test DB
    db_module._engine = engine
    db_module._session_factory = async_sessionmaker(
        bind=engine, expire_on_commit=False, class_=AsyncSession
    )
    yield engine
    db_module._engine = None
    db_module._session_factory = None
    await engine.dispose()


@pytest_asyncio.fixture(scope="function")
async def db_session(db_engine):
    factory = async_sessionmaker(db_engine, expire_on_commit=False)
    async with factory() as session:
        yield session


@pytest_asyncio.fixture(scope="function")
async def client(db_engine):
    """AsyncClient using the test SQLite database."""
    factory = async_sessionmaker(db_engine, expire_on_commit=False)

    async def override_get_db():
        async with factory() as session:
            try:
                yield session
                await session.commit()
            except Exception:
                await session.rollback()
                raise

    app.dependency_overrides[get_db] = override_get_db

    # lifespan=False skips startup/shutdown (avoids MQTT connect attempt in tests)
    transport = ASGITransport(app=app)
    async with AsyncClient(transport=transport, base_url="http://test") as ac:
        # Manually create tables since lifespan is skipped
        from app.database import Base
        async with db_engine.begin() as conn:
            await conn.run_sync(Base.metadata.create_all)
        yield ac

    app.dependency_overrides.clear()
