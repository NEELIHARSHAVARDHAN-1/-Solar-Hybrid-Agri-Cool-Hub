package com.agrihub.coolhub.util;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.TimeZone;

/**
 * Utility methods for formatting API ISO-8601 timestamps to display strings.
 */
public class DateUtils {

    private static final String ISO_FORMAT     = "yyyy-MM-dd'T'HH:mm:ssXXX";
    private static final String ISO_FORMAT_Z   = "yyyy-MM-dd'T'HH:mm:ss'Z'";
    private static final String DISPLAY_FULL   = "dd MMM yyyy HH:mm:ss";
    private static final String DISPLAY_SHORT  = "dd MMM HH:mm";
    private static final String DISPLAY_TIME   = "HH:mm";

    private DateUtils() {}

    /** Parses an ISO-8601 string and formats as "dd MMM yyyy HH:mm:ss". */
    public static String formatFull(String iso) {
        Date d = parse(iso);
        if (d == null) return iso != null ? iso : "--";
        return new SimpleDateFormat(DISPLAY_FULL, Locale.getDefault()).format(d);
    }

    /** Parses an ISO-8601 string and formats as "dd MMM HH:mm". */
    public static String formatShort(String iso) {
        Date d = parse(iso);
        if (d == null) return "--";
        return new SimpleDateFormat(DISPLAY_SHORT, Locale.getDefault()).format(d);
    }

    /** Parses an ISO-8601 string and formats as "HH:mm". */
    public static String formatTime(String iso) {
        Date d = parse(iso);
        if (d == null) return "--";
        return new SimpleDateFormat(DISPLAY_TIME, Locale.getDefault()).format(d);
    }

    /**
     * Returns true if the given ISO timestamp is older than 5 minutes,
     * or if the string is null/empty.
     */
    public static boolean isStale(String iso) {
        return isStale(iso, 5 * 60 * 1000L);
    }

    /**
     * Returns true if the given ISO timestamp is older than {@code thresholdMs} milliseconds,
     * or if the string is null/empty.
     */
    public static boolean isStale(String iso, long thresholdMs) {
        if (iso == null || iso.isEmpty()) return true;
        Date d = parse(iso);
        if (d == null) return true;
        return (System.currentTimeMillis() - d.getTime()) > thresholdMs;
    }

    private static Date parse(String iso) {
        if (iso == null || iso.isEmpty()) return null;
        // Try with timezone offset first, then UTC Z suffix
        for (String fmt : new String[]{ISO_FORMAT, ISO_FORMAT_Z}) {
            try {
                SimpleDateFormat sdf = new SimpleDateFormat(fmt, Locale.US);
                if (fmt.endsWith("'Z'")) sdf.setTimeZone(TimeZone.getTimeZone("UTC"));
                return sdf.parse(iso);
            } catch (Exception ignored) {
                // try next
            }
        }
        return null;
    }
}
