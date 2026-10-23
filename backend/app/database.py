from sqlalchemy.ext.asyncio import create_async_engine, async_sessionmaker, AsyncSession
from sqlalchemy.orm import DeclarativeBase
from .config import settings


class Base(DeclarativeBase):
    pass


def _make_engine():
    url = settings.database_url
    # SQLite (used in tests) does not support pool_size / max_overflow
    if url.startswith("sqlite"):
        return create_async_engine(
            url,
            echo=False,
            connect_args={"check_same_thread": False},
        )
    return create_async_engine(
        url,
        echo=False,
        pool_pre_ping=True,
        pool_size=10,
        max_overflow=20,
    )


# Lazily created — tests can patch settings.database_url before first use
_engine = None
_session_factory = None


def get_engine():
    global _engine
    if _engine is None:
        _engine = _make_engine()
    return _engine


def get_session_factory():
    global _session_factory
    if _session_factory is None:
        _session_factory = async_sessionmaker(
            bind=get_engine(),
            expire_on_commit=False,
            class_=AsyncSession,
        )
    return _session_factory


# Convenience alias used by mqtt_client (creates its own sessions)
class AsyncSessionLocal:
    def __new__(cls):
        return get_session_factory()()

    @classmethod
    def __call__(cls):
        return get_session_factory()()


async def get_db():
    """FastAPI dependency that yields an async database session."""
    factory = get_session_factory()
    async with factory() as session:
        try:
            yield session
            await session.commit()
        except Exception:
            await session.rollback()
            raise
