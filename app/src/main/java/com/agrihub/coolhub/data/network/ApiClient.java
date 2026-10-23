package com.agrihub.coolhub.data.network;

import okhttp3.OkHttpClient;
import okhttp3.logging.HttpLoggingInterceptor;
import retrofit2.Retrofit;
import retrofit2.converter.gson.GsonConverterFactory;

import java.util.concurrent.TimeUnit;

/**
 * Singleton Retrofit client.
 * Call {@link #create(String)} with the current backend base URL.
 * The instance is recreated when the URL changes (Settings screen).
 */
public class ApiClient {

    private static Retrofit retrofit;
    private static String currentBaseUrl = "";

    /** Returns an {@link AgriHubApi} instance for the given base URL. */
    public static AgriHubApi create(String baseUrl) {
        if (!baseUrl.equals(currentBaseUrl) || retrofit == null) {
            currentBaseUrl = baseUrl;
            retrofit = buildRetrofit(baseUrl);
        }
        return retrofit.create(AgriHubApi.class);
    }

    private static Retrofit buildRetrofit(String baseUrl) {
        HttpLoggingInterceptor logging = new HttpLoggingInterceptor();
        logging.setLevel(HttpLoggingInterceptor.Level.BASIC);

        OkHttpClient client = new OkHttpClient.Builder()
                .connectTimeout(15, TimeUnit.SECONDS)
                .readTimeout(15, TimeUnit.SECONDS)
                .writeTimeout(15, TimeUnit.SECONDS)
                .addInterceptor(logging)
                .build();

        // Ensure trailing slash for Retrofit base URL
        String url = baseUrl.endsWith("/") ? baseUrl : baseUrl + "/";

        return new Retrofit.Builder()
                .baseUrl(url)
                .client(client)
                .addConverterFactory(GsonConverterFactory.create())
                .build();
    }
}
