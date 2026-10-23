import { useSyncExternalStore, useCallback } from "react";
import { BASE_URL } from "./api";
import type { SSEEvent } from "./api";

type Listener = () => void;
const stores: Record<
  string,
  { data: SSEEvent | null; listeners: Set<Listener>; es: EventSource | null }
> = {};

function getStore(deviceId: string) {
  if (!stores[deviceId]) {
    stores[deviceId] = { data: null, listeners: new Set(), es: null };
  }
  return stores[deviceId];
}

function subscribe(deviceId: string, listener: Listener) {
  const store = getStore(deviceId);
  store.listeners.add(listener);

  if (!store.es) {
    const es = new EventSource(`${BASE_URL}/devices/${deviceId}/stream`);
    store.es = es;
    es.onmessage = (e) => {
      try {
        store.data = JSON.parse(e.data) as SSEEvent;
        store.listeners.forEach((l) => l());
      } catch {
        /* ignore parse errors */
      }
    };
  }

  return () => {
    store.listeners.delete(listener);
    if (store.listeners.size === 0 && store.es) {
      store.es.close();
      store.es = null;
    }
  };
}

export function useSSE(deviceId: string): SSEEvent | null {
  const getSnapshot = useCallback(() => getStore(deviceId).data, [deviceId]);
  const subscribeCallback = useCallback(
    (listener: Listener) => subscribe(deviceId, listener),
    [deviceId]
  );
  return useSyncExternalStore(subscribeCallback, getSnapshot, () => null);
}
