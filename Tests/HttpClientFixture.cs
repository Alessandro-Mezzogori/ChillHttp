namespace Tests;

public class HttpClientFixture
{
    public static HttpClient Client { get; } = InitClient();


    private static HttpClient InitClient()
    {
        var client = new HttpClient();
        client.BaseAddress = new Uri("http://localhost:8080");
        return client;
    }
}