using FluentAssertions;
using System.Net;

namespace Tests;

public class UnitTest1()
{
    [Fact]
    public async Task Test1()
    {
        var response = await HttpClientFixture.Client.GetAsync("/");

        response.StatusCode.Should().Be(HttpStatusCode.OK);
    }

    [Theory]
    [InlineData(500)]
    [InlineData(400)]
    [InlineData(404)]
    public async Task ShoudlError(int error)
    {
        var request = new HttpRequestMessage
        {
            RequestUri = HttpClientFixture.Client.BaseAddress,
            Method = HttpMethod.Get,
            Headers =
            {
                { "Test-Error", error.ToString() },
            },
        };

        var response = await HttpClientFixture.Client.SendAsync(request);
        response.StatusCode.Should().Be((HttpStatusCode)error);
    }
}
